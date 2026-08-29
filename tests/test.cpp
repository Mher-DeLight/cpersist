#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(cpersist, FileBufferWorks) {
    auto file = cpersist::File("myfile");
    file.write("mynumber", 3);

    EXPECT_EQ(file.read<int>("mynumber", 3), 3);
}