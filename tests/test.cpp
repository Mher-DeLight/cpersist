#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(Cpersist, FileBufferWorks) {
    auto file = cpersist::File("myfile");
    file.write("mynumber", 3);

    EXPECT_EQ(file.read<int>("mynumber", 3), 3);
}

TEST(Cpersist, CommitWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("commit_works", "bin");
    file.write("mynumber", 3);
    file.commit();

    EXPECT_TRUE(fs::exists(fs::path(cpersist::folderName) / "commit_works.bin"));
    fs::remove("savedata/commit_works.bin");
}

TEST(Cpersist, InitWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("init_works", "bin");
        file.write("mynumber", 5);
        file.commit();
    }
    {
        auto file = cpersist::File("init_works", "bin");
        EXPECT_EQ(file.read<int>("mynumber"), 5);
    }
    fs::remove("savedata/init_works.bin");
}

TEST(Cpersist, EncryptionWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("encr_works", "bin", "myencryptionkey");
        file.enable_encryption("myencryptionkey");
        file.write("mynumber", 5);
        file.commit();
    }
    {
        auto file = cpersist::File("encr_works", "bin", "myencryptionkey");
        EXPECT_EQ(file.read<int>("mynumber"), 5);
    }
    {
        EXPECT_ANY_THROW(auto file =
                             cpersist::File("encr_works", "bin", "myincorrectencryptionkey"));
    }
    fs::remove("savedata/encr_works.bin");
}

TEST(Cpersist, ContainsWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("contains_works");
        file.write("number", 3);
        file.write("othernumber", 5);
        EXPECT_TRUE(file.contains("number"));
        EXPECT_TRUE(file.contains({"number", "othernumber"}));
        EXPECT_TRUE(file.contains({"othernumber", "number"}));
        EXPECT_FALSE(file.contains("randomstring"));
        EXPECT_FALSE(file.contains({"foo", "boo"}));
        file.commit();
    }
    {
        auto file = cpersist::File("contains_works");
        EXPECT_TRUE(file.contains("number"));
        EXPECT_TRUE(file.contains({"number", "othernumber"}));
        EXPECT_TRUE(file.contains({"othernumber", "number"}));
        EXPECT_FALSE(file.contains("randomstring"));
        EXPECT_FALSE(file.contains({"foo", "boo"}));
    }
    fs::remove("savedata/contains_works.bin");
}

TEST(Cpersist, AutocommitWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("autocommit_works");
        file.enable_autocommit_on_destroy(true);
        file.write("number", 3);
        file.write("othernumber", 5);
    }
    {
        auto file = cpersist::File("autocommit_works");
        EXPECT_TRUE(file.contains("number"));
        EXPECT_TRUE(file.contains({"number", "othernumber"}));
    }
    fs::remove("savedata/autocommit_works.bin");
}

TEST(Cpersist, EraseWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("erase_works");
        EXPECT_ANY_THROW(file.erase("mynumber"));
        EXPECT_FALSE(file.contains("mynumber"));
        file.write("mynumber", 5);
        EXPECT_TRUE(file.contains("mynumber"));
        EXPECT_NO_THROW(file.erase("mynumber"));
        EXPECT_FALSE(file.contains("mynumber"));
    }
    {
        auto file = cpersist::File("erase_works");
        EXPECT_ANY_THROW(file.erase({"num1", "num2"}));

        EXPECT_FALSE(file.contains({"num1", "num2"}));

        file.write("num1", 5);
        file.write("num2", 3);

        EXPECT_TRUE(file.contains({"num1", "num2"}));
        EXPECT_NO_THROW(file.erase({"num1", "num2"}));
        EXPECT_FALSE(file.contains({"num1", "num2"}));
    }
}