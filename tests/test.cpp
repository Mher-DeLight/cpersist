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

TEST(CPersist, FileExtensionLoading) {
    const std::string filename = "extension_loading";

    saveMgr.enable_encryption(false);
    saveMgr.set_file_extension("dat");

    std::filesystem::remove("savedata/" + filename + ".dat");

    saveMgr.open(filename);
    saveMgr.write("number", 5);
    saveMgr.commit();

    // Remove the field from the in-memory buffer while keeping it on disk.
    saveMgr.erase("number");

    EXPECT_FALSE(saveMgr.contains("number"));

    // Setting the extension should reload existing files.
    saveMgr.set_file_extension("dat");

    EXPECT_TRUE(saveMgr.contains("number"));
    EXPECT_EQ(saveMgr.read<int>("number"), 5);

    std::filesystem::remove("savedata/" + filename + ".dat");
}
