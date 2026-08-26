#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

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

    saveManager.read_into_stream<int>(
        "number",
        stream);

    EXPECT_EQ(stream.str(), "3");
}
