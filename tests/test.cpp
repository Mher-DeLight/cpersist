#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>

struct templatestruct {
    int number = 0;
    templatestruct(int number_) : number(number_) {}
    templatestruct() = default;

    template <typename Archive> void archive(Archive& ar) {
        ar("number", number);
    }
};

TEST(Cpersist, FileBufferWorks) {
    auto file = cpersist::File("myfile");
    file.write("foo", 3);

    EXPECT_TRUE(file.contains("foo"));
    EXPECT_EQ(file.read<int>("foo"), 3);
    EXPECT_ANY_THROW(file.read<int>("bar"));
    EXPECT_EQ(file.read<int>("bar", 6), 6);
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
        auto file = cpersist::File("encr_works", "bin");
        file.enable_encryption("myencryptionkey"); // only works if the there is NO file on the disk
        // otherwise the constructor will throw because the OpenSSL authentication failed
        file.write("mynumber", 5);
        file.commit();
    }
    {
        auto file = cpersist::File("encr_works", "bin", "myencryptionkey");
        EXPECT_TRUE(file.contains("mynumber"));
        EXPECT_EQ(file.read<int>("mynumber"), 5);
        file.write("a", 3);
        file.commit();
    }
    {
        auto file = cpersist::File("encr_works", "bin", "myencryptionkey");
        EXPECT_EQ(file.read<int>("mynumber"), 5);
        EXPECT_TRUE(file.contains("a"));
        EXPECT_EQ(file.read<int>("a"), 3);
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
    // not very reliable, since commits may fail, and C++ doesn't usually let destructors throw just
    // like that
    // we do have atomic writing now, but stil not very safe.
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
        EXPECT_FALSE(file.erase("mynumber"));
        EXPECT_FALSE(file.contains("mynumber"));
        file.write("mynumber", 5);
        EXPECT_TRUE(file.contains("mynumber"));
        EXPECT_TRUE(file.erase("mynumber"));
        EXPECT_FALSE(file.contains("mynumber"));
    }
    {
        auto file = cpersist::File("erase_works");
        EXPECT_FALSE(file.erase({"num1", "num2"}));

        EXPECT_FALSE(file.contains({"num1", "num2"}));

        file.write("num1", 5);
        file.write("num2", 3);

        EXPECT_TRUE(file.contains({"num1", "num2"}));
        EXPECT_TRUE(file.erase({"num1", "num2"}));
        EXPECT_FALSE(file.contains({"num1", "num2"}));
    }
}
TEST(Cpersist, CopyingWorks) {
    namespace fs = std::filesystem;
    auto file1 = cpersist::File("copying1");
    file1.write("mynumber", 3);

    auto file2 = cpersist::CopyFile(file1, "copying2");
    EXPECT_TRUE(file2.contains("mynumber"));

    file2.write("foo", 5);
    EXPECT_EQ(file2.read<int>("mynumber"), 3);
    EXPECT_TRUE(file2.contains("mynumber"));
    EXPECT_TRUE(file2.contains("foo"));

    EXPECT_TRUE(file1.contains("mynumber"));
    EXPECT_FALSE(file1.contains("foo"));

    {
        auto file3 = cpersist::CopyFile(file1, "copying3", "bin", "someencryptionkey");
        file3.write("d", 6);
        file3.commit();
    }
    auto file3 = cpersist::File("copying3", "bin", "someencryptionkey");
    EXPECT_TRUE(file3.contains("mynumber"));
    EXPECT_TRUE(file3.contains("d"));

    EXPECT_EQ(file3.read<int>("mynumber"), 3);
    EXPECT_EQ(file3.read<int>("d"), 6);
    fs::remove("savedata/copying3.bin");
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
TEST(Cpersist, StashesWork) {
    struct mystruct {
        int number = 0;
        bool callDestruct = true;
        mystruct(int number_) : number(number_) {}
        ~mystruct() noexcept(false) {
            if (callDestruct)
                throw std::runtime_error("destructor called!");
        }
    };

    { cpersist::Stash<mystruct> foo("foo", 7); }
    mystruct* foo = &cpersist::LoadStash<mystruct>("foo");
    EXPECT_NE(foo, nullptr);
    EXPECT_EQ(foo->number, 7);
    foo->callDestruct = false;

    cpersist::FreeStash<mystruct>("foo");

    { cpersist::Stash<mystruct*> foo("foo", new mystruct(3)); }
    foo = cpersist::LoadStash<mystruct*>("foo");
    EXPECT_NE(foo, nullptr);
    EXPECT_EQ(foo->number, 3);
    foo->callDestruct = false;

    cpersist::FreeStash<mystruct*>("foo");
}
TEST(Cpersist, SyncWorks) {
    // write if not present in the file
    // otherwise read
    auto file = cpersist::File("sync_works");
    int mynumber = 5;
    file.sync("num", mynumber);
    EXPECT_TRUE(file.contains("num"));
    EXPECT_EQ(file.read<int>("num"), 5);

    mynumber = 7;
    file.sync("num", mynumber);
    EXPECT_TRUE(file.contains("num"));
    EXPECT_EQ(mynumber, 5);
}
TEST(Cpersist, ReadIntoWorks) {
    auto file = cpersist::File("readinto_works");
    int mynumber = 5;
    file.write("num", mynumber);
    EXPECT_TRUE(file.contains("num"));
    EXPECT_EQ(file.read<int>("num"), 5);

    mynumber = 13;
    file.read_into("num", mynumber);
    EXPECT_TRUE(file.contains("num"));
    EXPECT_EQ(mynumber, 5);

    EXPECT_ANY_THROW(file.read_into("bar", mynumber));
    file.read_into("bar", mynumber, std::optional<int>{7});
    EXPECT_EQ(mynumber, 7);
}
TEST(Cpersist, ArchivesWork) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("archives_work");
    templatestruct obj(3);
    file.write("obj", obj);
    file.commit();

    file.refresh();

    EXPECT_EQ(file.read<templatestruct>("obj").number, 3);
    fs::remove("savedata/archives_work.bin");
}
TEST(Cpersist, DiscardWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("discard_works");
    file.write("a", 5);
    file.commit();

    file.write("a", 3);
    file.write("b", 2);
    file.discard();

    EXPECT_EQ(file.read<int>("a"), 5);
    EXPECT_FALSE(file.contains("b"));
    fs::remove("savedata/discard_works.bin");
}
TEST(Cpersist, WriteStashWorks) {
    {
        // make the stash here
        cpersist::Stash<templatestruct> foo("writestash", 7);
    }
    auto file = cpersist::File("writestash_works");
    file.writeStash<templatestruct>("writestash");
    EXPECT_TRUE(file.contains("writestash"));
    EXPECT_EQ(file.read<templatestruct>("writestash").number, 7);

    cpersist::FreeStash<templatestruct>("writestash");
}
TEST(Cpersist, WriteProposalsWork) {
    auto file = cpersist::File("debatewrite_works");
    int number = 5;

    // accept test
    {
        auto proposal = file.proposeWrite("foo", number);
        EXPECT_FALSE(file.contains("foo"));
        proposal.accept();
        EXPECT_TRUE(file.contains("foo"));
        EXPECT_EQ(file.read<int>("foo"), 5);
    }
    // reject test
    {
        auto proposal = file.proposeWrite("bar", number);
        EXPECT_FALSE(file.contains("bar"));
        proposal.reject();
        EXPECT_FALSE(file.contains("bar"));
    }
    // multiple proposals
    {
        auto proposal1 = file.proposeWrite("a", 5);
        auto proposal2 = file.proposeWrite("b", 3);
        EXPECT_FALSE(file.contains("a"));
        EXPECT_FALSE(file.contains("b"));
        proposal1.accept();
        proposal2.reject();
        EXPECT_TRUE(file.contains("a"));
        EXPECT_EQ(file.read<int>("a"), 5);
        EXPECT_FALSE(file.contains("b"));
    }
}
TEST(Cpersist, SchemaVersionsWork) {
    namespace fs = std::filesystem;
    {
        auto file = cpersist::File("schemaversion_works");
        file.set_schema_version(5);
        EXPECT_EQ(file.get_schema_buffer_version(), 5);
        file.commit();
    }
    {
        auto file = cpersist::File("schemaversion_works");
        EXPECT_ANY_THROW(file.schema_standard(10));
    }
    {
        auto file = cpersist::File("schemaversion_works");
        EXPECT_EQ(file.get_schema_file_version(), 5);
    }
    {
        auto file = cpersist::File("schemaversion_works");
        file.set_schema_version(6);
        EXPECT_EQ(file.get_schema_buffer_version(), 6);
        EXPECT_EQ(file.get_schema_file_version(), 5);
        file.commit();
        EXPECT_EQ(file.get_schema_file_version(), 6);
    }
    fs::remove("savedata/schemaversion_works.bin");
}
TEST(Cpersist, ClearWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("clear_works");
    file.write("a", 5);
    EXPECT_TRUE(file.contains("a"));
    file.clear();
    EXPECT_FALSE(file.contains("a"));

    file.write("a", 5);
    file.commit();
    EXPECT_TRUE(file.contains("a"));

    file.clear();
    file.refresh();
    EXPECT_TRUE(file.contains("a"));

    file.write("a", 5);
    file.commit();
    file.clear();
    file.refresh();
    EXPECT_TRUE(file.contains("a"));
    fs::remove("savedata/clear_works.bin");
}
TEST(Cpersist, OperatorBracketWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("myfile");
    file["a"] = 5;
    EXPECT_TRUE(file.contains("a"));
    EXPECT_EQ(file.read<int>("a"), 5);
    file.commit();
    file.refresh();
    EXPECT_TRUE(file.contains("a"));
    EXPECT_EQ(file.read<int>("a"), 5);
    file["a"] = 3;
    EXPECT_TRUE(file.contains("a"));
    EXPECT_EQ(file.read<int>("a"), 3);
    fs::remove("savedata/myfile.bin");
}
TEST(Cpersist, UnorderedMapWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("umap_works");
    std::unordered_map<std::string, int> map = {
        {"key1", 3}, {"key2", 5}, {"key3", 14}, {"key4", 12}};
    file.write("umap", map);
    EXPECT_TRUE(file.contains("umap"));

    auto result = file.read<std::unordered_map<std::string, int>>("umap");
    EXPECT_EQ(result["key1"], map["key1"]);
    EXPECT_EQ(result["key2"], map["key2"]);
    EXPECT_EQ(result["key3"], map["key3"]);
    EXPECT_EQ(result["key4"], map["key4"]);
    file.commit();
    file.refresh();

    EXPECT_TRUE(file.contains("umap"));

    auto newresult = file.read<std::unordered_map<std::string, int>>("umap");
    EXPECT_EQ(newresult["key1"], map["key1"]);
    EXPECT_EQ(newresult["key2"], map["key2"]);
    EXPECT_EQ(newresult["key3"], map["key3"]);
    EXPECT_EQ(newresult["key4"], map["key4"]);
    fs::remove("savedata/umap_works.bin");
}
TEST(Cpersist, StashConversionWorks) {
    std::string& mystring = cpersist::Stash<std::string>("stashconv_stash", "foo");
    mystring = "bar";
    EXPECT_EQ(cpersist::LoadStash<std::string>("stashconv_stash"), "bar");

    std::string myotherstring = cpersist::Stash<std::string>("stashconv2_stash", "foo");
    myotherstring = "bar";
    EXPECT_EQ(cpersist::LoadStash<std::string>("stashconv2_stash"), "foo");

    cpersist::FreeStash<std::string>("stashconv_stash");
    cpersist::FreeStash<std::string>("stashconv2_stash");
}
