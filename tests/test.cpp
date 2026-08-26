#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>
#include <map>

TEST(CPersist, IOWorks) {
    SaveManager saveManager;

    saveManager.enable_encryption(true);
    saveManager.set_encryption_key("myencryptionkey");

    saveManager.open("myfile");

    saveManager.write("number", 3);
    saveManager.commit();

    int mynumber = saveManager.read<int>("number");
    EXPECT_EQ(mynumber, 3);

    mynumber = 0;

    saveManager.read_into("number", mynumber);

    EXPECT_EQ(mynumber, 3);

    std::stringstream stream;
    saveManager.read_into_stream<int>("number", stream);
    EXPECT_EQ(stream.str(), "3");
    std::filesystem::remove("savedata/myfile.bin");
}

TEST(CPersist, InvalidMagicHeader) {
    SaveManager saveManager;

    saveManager.open("invalid_magic_header");
    saveManager.commit();

    std::filesystem::path filePath = "savedata/invalid_magic_header.bin";

    std::fstream file(filePath, std::ios::in | std::ios::out | std::ios::binary);
    ASSERT_TRUE(file.is_open());

    const char invalidHeader[] = "INVALID_MAGIC_HEADER";
    file.seekp(0);
    file.write(invalidHeader, sizeof(invalidHeader) - 1);
    file.close();

    EXPECT_THROW(saveManager.init(), std::runtime_error);

    std::filesystem::remove(filePath);
}

TEST(CPersist, FileExtensionLoading) {
    SaveManager saveManager;
    const std::string filename = "extension_loading";

    saveManager.enable_encryption(false);
    saveManager.set_file_extension("dat");

    std::filesystem::remove("savedata/" + filename + ".dat");

    saveManager.open(filename);
    saveManager.write("number", 5);
    saveManager.commit();

    // Remove the field from the in-memory buffer while keeping it on disk.
    saveManager.erase("number");

    EXPECT_FALSE(saveManager.contains("number"));

    // Setting the extension should reload existing files.
    saveManager.set_file_extension("dat");

    EXPECT_TRUE(saveManager.contains("number"));
    EXPECT_EQ(saveManager.read<int>("number"), 5);

    std::filesystem::remove("savedata/" + filename + ".dat");
}

struct mystruct {
    int number = 5;

    template <typename Archive> void archive(Archive& ar) {
        ar("number", number);
    }
    mystruct(int number_) : number(number_) {}
    mystruct() = default;
};
TEST(CPersist, ArchivesWork) {
    mystruct myobj(5);

    SaveManager saveManager;
    const std::string filename = "archive_test";
    saveManager.enable_encryption(false);

    std::filesystem::remove("savedata/" + filename + ".bin");

    saveManager.open(filename);
    saveManager.write("obj", myobj);
    saveManager.commit();

    saveManager.erase("obj");

    saveManager.init(); // init again to read files

    EXPECT_EQ(saveManager.read<mystruct>("obj").number, 5);

    std::filesystem::remove("savedata/" + filename + ".bin");
}

TEST(CPersist, StdMapSerialization) {
    std::map<std::string, int> testMap = {{"one", 1}, {"two", 2}, {"three", 3}};
    SaveManager saveManager;
    const std::string filename = "stdmap_test";

    std::filesystem::remove("savedata/" + filename + ".bin");

    saveManager.open(filename);
    saveManager.write("map", testMap);
    saveManager.commit();

    saveManager.erase("map");

    saveManager.init(); // init again to read files

    auto readmap = saveManager.read<std::map<std::string, int>>("map");
    EXPECT_EQ(readmap["one"], 1);
    EXPECT_EQ(readmap["two"], 2);
    EXPECT_EQ(readmap["three"], 3);

    std::filesystem::remove("savedata/" + filename + ".bin");
}