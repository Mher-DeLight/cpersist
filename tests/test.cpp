#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(CPersist, IOWorks) {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("wowyoureallyfoundit");

    saveMgr.open("myfile");

    saveMgr.write("number", 3);
    saveMgr.commit();

    int mynumber = saveMgr.read<int>("number");
    EXPECT_EQ(mynumber, 3);

    mynumber = 0;
    saveMgr.read_into("number", mynumber);
    EXPECT_EQ(mynumber, 3);

    std::stringstream stream;
    saveMgr.read_into_stream<int>("number", stream);
    EXPECT_EQ(stream.str(), "3");
}