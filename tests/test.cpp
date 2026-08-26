#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

TEST(CPersist, IOWorks) {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("myencryptionkey");

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

TEST(CPersist, InvalidMagicHeader) {
    saveMgr.open("invalid_magic_header");
    saveMgr.commit();

    std::filesystem::path filePath = "savedata/invalid_magic_header.bin";

    std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());

    const char invalidHeader[] = "INVALID_MAGIC_HEADER";
    file.seekp(0);
    file.write(invalidHeader, sizeof(invalidHeader) - 1);
    file.close();

    EXPECT_THROW(saveMgr.init(), std::runtime_error);

    std::filesystem::remove(filePath);
}