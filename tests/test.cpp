#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(cpersist, FileBufferWorks) {
    auto file = cpersist::File("myfile");
    file.write("mynumber", 3);

    EXPECT_EQ(file.read<int>("mynumber", 3), 3);
}

TEST(cpersist, CommitWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("commit_works", false, ".bin");
    file.write("mynumber", 3);
    file.commit();

    EXPECT_TRUE(fs::exists(fs::path(cpersist::folderName) / "commit_works.bin"));
    fs::remove("commit_works.bin");
}