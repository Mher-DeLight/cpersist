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