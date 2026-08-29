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
    auto file = cpersist::File("commit_works", false, "bin");
    file.write("mynumber", 3);
    file.commit();

    EXPECT_TRUE(fs::exists(fs::path(cpersist::folderName) / "commit_works.bin"));
    fs::remove("savedata/commit_works.bin");
}

TEST(Cpersist, InitWorks) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("init_works", false, "bin");
        file.write("mynumber", 5);
        file.commit();
    }
    {
        auto file = cpersist::File("init_works", false, "bin");
        EXPECT_EQ(file.read<int>("mynumber"), 5);
    }
    fs::remove("savedata/init_works.bin");
}