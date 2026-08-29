# Quick Documentation
This document covers some functions to get you up-and-started with cpersist. First, see our [installation guide](README.md/#installation).
## Files & Buffers
```cpp
cpersist::File(const std::string& filename, const std::string& extension, const std::string& encryptionKey)
```
- `filename` is the name of the file to be created.
- `extension` is that file's extension. `.bin` by default.
- `encryptionKey` is the key used to encrypt the file.

Returns a `cpersist::File` instance that can be used to save, load, and manipulate data. Basically the most essential structure in cpersist.

```cpp
cpersist::File.write(const std::string& fieldname, const T& fieldvalue)
```
- `fieldname` is the label under which to save the value.
- `fieldvalue` is a value of an arbitrary type to be saved.

Writes the data `fieldvalue` to the file buffer under the name `fieldname`.
```cpp
cpersist::File.commit()
```
Dumps file buffer into a file with the name `filename.extension` in the `savedata/` folder.

```cpp
cpersist::File.read<T>(const std::string& fieldname, std::optional<T> defaultValue)
```
- `fieldname` is the whose data should be read.
- `defaultValue` is the value returned if the data is not found. Passing this is optional.

Searches for the data in the file buffer and returns it if found. If not found, it returns `defaultValue`. If no `defaultValue` is passed, it throws.

```cpp
cpersist::File.contains(const std::string& fieldname)
```
- `fieldname` is the field whose name is to be search.

Returns `true` if a field of name `fieldname` is found in the file buffer. `false` otherwise.

### Quick Example
```cpp
{
    auto file = cpersist::File("myfile");
    file.write("num", 3);
    file.commit();
}
// some time later, maybe in another session
{
    auto file = cpersist::File("myfile");
    int number = file.read<int>("num");
}
```

## Stashes
A stash is a wrapper type whose data is not destroyed on scope exit and can be accessed, allowing the data to bypass usual scope rules. Here's an example:
```cpp
{
    struct mystruct {
        int number = 0;
        mystruct(int number_) : number(number_) {}
        mystruct() = default;

        ~mystruct() noexcept(false) {
            throw std::runtime_error("destructor called!");
        }
    };

    {
        cpersist::Stash<mystruct> foo("obj", 5);
    } // usually destroyed here with normal scope rules
      // but it doesn't throw thanks to cpersist::Stash
    
    mystruct bar = *cpersist::LoadStash<mystruct>("obj");
    std::cout << foo.number << std::endl; // outputs 5
} // throws here, because bar's destructor is called
```
The snippet might be dense, but what essential happens is that we create a class that throws when destroyed so we know when it is destroyed. Then we define `cpersist::Stash<mystruct> foo` in a scope where it is destroyed under the label `"obj"`. Then we laod it into `bar` with `cpersist::LoadStash<mystruct>("obj")` and keep using it as a normal object.

As you can see, thanks to this method, we were able to use an object defined in a lower scope inside a scope higher than itself. Note that this method also works for memory allocated on the heap.