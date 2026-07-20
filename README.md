# cpersist

**cpersist** is a lightweight C++ library for saving and loading data between runs of your program. Instead of writing file handling code every time you need persistent storage, cpersist provides a simple interface for storing values under labels and retrieving them later.

The library is designed with simplicity in mind. Whether you're making a small game, a command-line utility, or a personal project, cpersist aims to make persistence easy without requiring knowledge of serialization formats or complex file I/O.

## Features

cpersist is currently in a very early stage, meaning features are minimal. Current features include:

* Save data under a string label.
* Load data using its label.
* Automatically persist data between program sessions.
* Customize the file format used for storage.
* Customize how files are read and written.
* Lightweight with minimal dependencies.
* Custom writing and loading for custom classes
* Simple and beginner friendly API.

## Example

```cpp
saveMgr.open("playerdata");
int high_score = 10;
if (!saveMgr.contains("highscore")) {
    saveMgr.write("highscore", high_score); // save if not saved already
    saveMgr.commit();
} else {
    high_score = saveMgr.read<int>("highscore");
}
```