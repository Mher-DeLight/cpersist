<div align="center">
    <img src="assets/logo.svg" style="width: 150px; height: 150px;">
</div>

<div align="center">
    
# cpersist
    
</div>

> [!WARNING]
> cpersist is still in the Alpha pre-release. Although the project is usable, backward compatibility is still not in mind.

**cpersist** is a lightweight C++20 library for saving and loading data between runs of your program. Instead of writing file handling code every time you need persistent storage, cpersist provides a simple interface for storing values under labels and retrieving them later.

The library is designed with simplicity in mind. Whether you're making a small game, a command-line utility, or a personal project, cpersist aims to make persistence easy without requiring knowledge of serialization formats or complex file I/O.

## Features

cpersist is currently in an early stage, meaning features are minimal. Current features include:

* Save data under a string label.
* Load data using its label.
* Automatically persist data between program sessions.
* Customize the file format used for storage.
* Customize how files are read and written.
* Lightweight with minimal dependencies.
* Custom writing and loading for custom classes
* Simple and beginner-friendly API.

## Example

```cpp
auto file = cpersist::File("playerdata");
int high_score = 10;
if (!file.contains("highscore")) {
    file.write("highscore", high_score); // save if not saved already
    file.commit();
} else {
    high_score = file.read<int>("highscore");
}
```
Or, its equivalent:
```cpp
auto file = cpersist::File("playerdata");
int high_score = 10;
file.sync("highscore", highscore);
file.commit();
```
## Serialization
The library currently supports the serialization of:
- Trivially serializable types (e.g. `int`, `float`, `bool`, `char`)
- Custom classes (via Archives)
- `std::string`
- `std::vector`
- `std::map`
- `std::unordered_map`
- `std::array`
- Hopefully more to come

## Installation
### Quick Installation
Go to the top level of your repository, where `include/` is, and run this command:
```bash
curl -fsSL https://raw.githubusercontent.com/Mher-DeLight/cpersist/main/install.sh | sh
```
Then add the lines that the output tells you to add to your CMakeLists.txt, which are:
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
target_link_libraries(your_cmake_target PRIVATE ${CMAKE_SOURCE_DIR}/include/cpersist/src/cpersist.a OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(your_cmake_target PRIVATE ${CMAKE_SOURCE_DIR}/include/cpersist/include)
```
Note that this method is currently only supported for x86-64 Linux.
### Latest Release
Download the `.tar.gz` that was attached to the release. In your CMake directory, make sure you have a folder called `include/`. Inside that folder, create another folder called `cpersist/` and export the `.tar.gz` there. Then, add the following in your CMakeLists.txt:
```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)

target_link_libraries(your_cmake_target PRIVATE ${CMAKE_SOURCE_DIR}/include/cpersist/src/cpersist.a OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(your_cmake_target PRIVATE ${CMAKE_SOURCE_DIR}/include/cpersist/include)
```
### Install from Head
Go to your project directory, and make sure you have `include/`. Then inside that folder, run:
```bash
git clone https://www.github.com/Mher-DeLight/cpersist
```
Then go to your project's CMakeLists.txt, which is on the same level as the top-level `include/` and add:
```cmake
add_subdirectory(include/cpersist)
target_link_libraries(your_cmake_target PRIVATE cpersist)
```
Then in your C++ file, you can do:
```cpp
#include <cpersist.h>
```
Then you can use cpersist.

## Documentation
We have a small [quickstart documentation file](DOCUMENTATION.md). For more complicated queries, please check out our [wiki](https://github.com/Mher-DeLight/cpersist/wiki).
> [!NOTE]
> The wiki is currently very early Work-in-Progress. Right now, it's probably better if you just refer to the normal [documentation file.](DOCUMENTATION.md)
