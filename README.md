# cpersist

**cpersist** is a lightweight C++ library for saving and loading data between runs of your program. Instead of writing file handling code every time you need persistent storage, cpersist provides a simple interface for storing values under labels and retrieving them later.

The library is designed with simplicity in mind. Whether you're making a small game, a command-line utility, or a personal project, cpersist aims to make persistence easy without requiring knowledge of serialization formats or complex file I/O.

## Why use cpersist?

Saving application data is something almost every program eventually needs to do. Unfortunately, writing your own save system often means dealing with:

* Opening and closing files
* Choosing a file format
* Parsing data back into your program
* Handling errors
* Organizing save files

cpersist abstracts away most of this boilerplate so you can focus on your application instead of storage.

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

## Example use cases

cpersist is useful for storing things like:

* User settings
* Window size and position
* High scores
* Game save data
* Configuration values
* Recently opened files
* Application preferences
* Cached values

## Example

```cpp
SaveManager sm;
int high_score = 3;
sm.create_new_file("scores");
sm.change_file("scores");
if (!sm.file_contains_data("highscore")) {
    sm.write("highscore", high_score); // save if not saved already
    sm.commit();
} else {
    high_score = sm.read<int>("highscore");
}
```