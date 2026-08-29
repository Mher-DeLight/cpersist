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
TEST(Cpersist, CopyingWorks) {
    namespace fs = std::filesystem;
    auto file1 = cpersist::File("copying1");
    file1.write("mynumber", 3);

    auto file2 = cpersist::CopyFile(file1);
    EXPECT_TRUE(file2.contains("mynumber"));

    file2.write("foo", 5);
    EXPECT_EQ(file2.read<int>("mynumber"), 3);
    EXPECT_TRUE(file2.contains("mynumber"));
    EXPECT_TRUE(file2.contains("foo"));

    EXPECT_TRUE(file1.contains("mynumber"));
    EXPECT_FALSE(file1.contains("foo"));
}
TEST(Cpersist, MergeWorks) {
    namespace fs = std::filesystem;
    auto file1 = cpersist::File("file1");
    file1.write("a", 3);
    file1.write("b", 1);
    {
        auto file2 = cpersist::File("file2");
        file2.write("b", 5);
        file2.write("c", 7);
        file1.merge(file2);
    }

    EXPECT_TRUE(file1.contains("a"));
    EXPECT_TRUE(file1.contains("b"));
    EXPECT_TRUE(file1.contains("c"));

    EXPECT_EQ(file1.read<int>("a"), 3);
    EXPECT_EQ(file1.read<int>("b"), 5);
    EXPECT_EQ(file1.read<int>("c"), 7);
}
