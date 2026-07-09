#pragma once
#include <string>

// debugging
void say_hello();

class SaveManager {
private:
    std::string current_file;
public:
    void change_file(const std::string& new_file);     // changes the current file
    void create_new_file(const std::string& new_file);        // creates a new file
    void use_new_file(const std::string& new_file);    // creates a new file and switches to it
};