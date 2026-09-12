#include <cpersist.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory_resource>
#include <sstream>

struct templatestruct {
    int number = 0;
    templatestruct(int number_) : number(number_) {}
    templatestruct() = default;

    template <typename Archive> void archive(Archive& ar) {
        ar("number", number);
    }
};

namespace cpersist {
template <> struct Serializer<std::pmr::string> {
    static void write(std::ostream& os, const std::pmr::string& value) {
        uint32_t size = static_cast<uint32_t>(value.size());
        Serializer<uint32_t>::write(os, size);
        os.write(value.data(), size);
    }

    static void read(std::istream& is, std::pmr::string& value) {
        uint32_t size;
        Serializer<uint32_t>::read(is, size);
        value.resize(size);
        is.read(value.data(), size);
    }
};
} // namespace cpersist

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

    EXPECT_NO_THROW(file["a"]);
    EXPECT_EQ(file["a"].get<int>(), 5);

    file.commit();

    EXPECT_NO_THROW(file["a"]);
    EXPECT_EQ(file["a"].get<int>(), 5);

    file.refresh();
    EXPECT_TRUE(file.contains("a"));
    EXPECT_EQ(file.read<int>("a"), 5);

    EXPECT_NO_THROW(file["a"]);
    EXPECT_EQ(file["a"].get<int>(), 5);

    file["a"] = 3;
    EXPECT_TRUE(file.contains("a"));
    EXPECT_EQ(file.read<int>("a"), 3);

    EXPECT_NO_THROW(file["a"]);
    EXPECT_EQ(file["a"].get<int>(), 3);

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
TEST(Cpersist, SetWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("set_works");
    const std::set<std::string> populated = {"apple", "banana", "orange"};
    const std::set<std::string> empty;

    file.write("populated", populated);
    file.write("empty", empty);
    EXPECT_EQ(file.read<std::set<std::string>>("populated"), populated);
    EXPECT_EQ(file.read<std::set<std::string>>("empty"), empty);

    file.commit();
    file.refresh();

    EXPECT_EQ(file.read<std::set<std::string>>("populated"), populated);
    EXPECT_EQ(file.read<std::set<std::string>>("empty"), empty);
    fs::remove("savedata/set_works.bin");
}
TEST(Cpersist, OptionalWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("optional_works");
    const std::optional<int> number = 42;
    const std::optional<std::string> populated = "saved value";
    const std::optional<std::string> empty;
    const std::vector<std::optional<std::string>> nested = {
        std::optional<std::string>("first"), std::nullopt, std::optional<std::string>("third")};

    file.write("number", number);
    file.write("populated", populated);
    file.write("empty", empty);
    file.write("nested", nested);
    EXPECT_EQ(file.read<std::optional<int>>("number"), number);
    EXPECT_EQ(file.read<std::optional<std::string>>("populated"), populated);
    EXPECT_EQ(file.read<std::optional<std::string>>("empty"), empty);
    EXPECT_EQ(file.read<std::vector<std::optional<std::string>>>("nested"), nested);

    file.commit();
    file.refresh();

    EXPECT_EQ(file.read<std::optional<int>>("number"), number);
    EXPECT_EQ(file.read<std::optional<std::string>>("populated"), populated);
    EXPECT_EQ(file.read<std::optional<std::string>>("empty"), empty);
    EXPECT_EQ(file.read<std::vector<std::optional<std::string>>>("nested"), nested);
    fs::remove("savedata/optional_works.bin");
}
TEST(Cpersist, EmptyOptionalClearsExistingValue) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    const std::optional<std::string> empty;
    cpersist::Serializer<std::optional<std::string>>::write(stream, empty);

    std::optional<std::string> result = "stale value";
    stream.seekg(0);
    cpersist::Serializer<std::optional<std::string>>::read(stream, result);

    EXPECT_EQ(result, std::nullopt);
}
TEST(Cpersist, TruncatedOptionalDiscriminatorDoesNotMutateDestination) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    std::optional<std::string> result = "stale value";

    EXPECT_THROW(cpersist::Serializer<decltype(result)>::read(stream, result), std::runtime_error);

    EXPECT_TRUE(stream.fail());
    EXPECT_EQ(result, "stale value");
}
TEST(Cpersist, InvalidOptionalDiscriminatorDoesNotMutateDestination) {
    std::pmr::monotonic_buffer_resource targetResource;
    std::optional<std::pmr::string> result(std::in_place, "stale value", &targetResource);
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<uint8_t>::write(stream, uint8_t{2});
    stream.seekg(0);

    EXPECT_THROW(cpersist::Serializer<decltype(result)>::read(stream, result), std::runtime_error);

    EXPECT_TRUE(stream.fail());
    EXPECT_EQ(*result, "stale value");
    EXPECT_EQ(result->get_allocator().resource(), &targetResource);
}
TEST(Cpersist, NestedOptionalRejectsInvalidDiscriminator) {
    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<uint32_t>::write(stream, uint32_t{1});
    cpersist::Serializer<uint8_t>::write(stream, uint8_t{2});
    stream.seekg(0);

    std::vector<std::optional<int>> result;
    EXPECT_THROW(cpersist::Serializer<decltype(result)>::read(stream, result), std::runtime_error);
    EXPECT_TRUE(stream.fail());
}
TEST(Cpersist, OptionalReadsExistingBooleanDiscriminatorFormat) {
    std::stringstream presentStream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<bool>::write(presentStream, true);
    cpersist::Serializer<int>::write(presentStream, 42);
    presentStream.seekg(0);

    std::optional<int> presentResult;
    cpersist::Serializer<decltype(presentResult)>::read(presentStream, presentResult);
    EXPECT_EQ(presentResult, 42);

    std::stringstream emptyStream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<bool>::write(emptyStream, false);
    emptyStream.seekg(0);

    std::optional<int> emptyResult = 7;
    cpersist::Serializer<decltype(emptyResult)>::read(emptyStream, emptyResult);
    EXPECT_EQ(emptyResult, std::nullopt);
}
TEST(Cpersist, TrivialOptionalsUseSemanticFormatWhenNested) {
    static_assert(std::is_trivially_copyable_v<std::optional<int>>);
    static_assert(!cpersist::detail::isRawCopyEligible<std::optional<int>>);
    static_assert(!cpersist::detail::isRawCopyEligible<std::array<std::optional<int>, 3>>);

    const std::vector<std::optional<int>> vectorValues = {7, std::nullopt, -4};
    std::stringstream elementStream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<std::optional<int>>::write(elementStream, vectorValues.front());
    EXPECT_EQ(elementStream.str().size(), sizeof(uint8_t) + sizeof(int));
    EXPECT_EQ(static_cast<uint8_t>(elementStream.str().front()), uint8_t{1});

    std::stringstream vectorStream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<std::vector<std::optional<int>>>::write(vectorStream, vectorValues);

    std::stringstream expectedVector(std::ios::in | std::ios::out | std::ios::binary);
    const uint32_t vectorSize = static_cast<uint32_t>(vectorValues.size());
    cpersist::Serializer<uint32_t>::write(expectedVector, vectorSize);
    for (const auto& value : vectorValues) {
        cpersist::Serializer<std::optional<int>>::write(expectedVector, value);
    }
    EXPECT_EQ(vectorStream.str(), expectedVector.str());

    std::vector<std::optional<int>> vectorResult;
    vectorStream.seekg(0);
    cpersist::Serializer<decltype(vectorResult)>::read(vectorStream, vectorResult);
    EXPECT_EQ(vectorResult, vectorValues);

    const std::array<std::optional<int>, 3> arrayValues = {11, std::nullopt, 13};
    std::stringstream arrayStream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<std::array<std::optional<int>, 3>>::write(arrayStream, arrayValues);

    std::stringstream expectedArray(std::ios::in | std::ios::out | std::ios::binary);
    for (const auto& value : arrayValues) {
        cpersist::Serializer<std::optional<int>>::write(expectedArray, value);
    }
    EXPECT_EQ(arrayStream.str(), expectedArray.str());

    std::array<std::optional<int>, 3> arrayResult;
    arrayStream.seekg(0);
    cpersist::Serializer<decltype(arrayResult)>::read(arrayStream, arrayResult);
    EXPECT_EQ(arrayResult, arrayValues);
}
TEST(Cpersist, PresentOptionalPreservesContainedAllocator) {
    std::pmr::monotonic_buffer_resource sourceResource;
    std::pmr::monotonic_buffer_resource targetResource;
    const std::optional<std::pmr::string> source(std::in_place, "new value", &sourceResource);
    std::optional<std::pmr::string> result(std::in_place, "stale value", &targetResource);

    std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
    cpersist::Serializer<std::optional<std::pmr::string>>::write(stream, source);
    stream.seekg(0);
    cpersist::Serializer<decltype(result)>::read(stream, result);

    EXPECT_EQ(*result, "new value");
    EXPECT_EQ(result->get_allocator().resource(), &targetResource);
}
TEST(Cpersist, PairWorks) {
    auto file = cpersist::File("pair_works");
    const std::pair<int, std::string> pair = {42, "cpersist"};
    file.write("pair", pair);

    auto result = file.read<std::pair<int, std::string>>("pair");
    EXPECT_EQ(result, pair);
}
TEST(Cpersist, NestedPairWorks) {
    auto file = cpersist::File("nested_pair_works");
    const std::pair<std::string, std::vector<int>> pair = {"values", {1, 2, 3}};
    file.write("pair", pair);

    auto result = file.read<std::pair<std::string, std::vector<int>>>("pair");
    EXPECT_EQ(result, pair);
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
TEST(Cpersist, FileSystemPathWorks) {
    namespace fs = std::filesystem;
    auto file = cpersist::File("path_works");
    fs::path original_path = "/tmp/test_cpersist_path";
    file.write("my_path", original_path);
    file.commit();
    
    file.refresh();
    EXPECT_TRUE(file.contains("my_path"));
    fs::path read_path = file.read<fs::path>("my_path");
    EXPECT_EQ(read_path, original_path);
    
    fs::remove("savedata/path_works.bin");
}
