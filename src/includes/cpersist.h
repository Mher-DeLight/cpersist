#pragma once
#include <string>
#include <unordered_set>


class SaveManager {
private:
    std::string current_file;
    std::unordered_set<std::string> files;
public:
    bool filename_fits_standards(const std::string& filename); // check if the filename fits the naming standards
    void make_filename_safe(std::string& filename);

    bool change_file(const std::string& new_file);            // changes the current file
    void change_file_safe(const std::string& new_file);       // creates file if it doesn't exist, then moves to it in either case
    bool create_new_file(const std::string& new_file);        // creates a new file

    void log_filenames();
    void log_current_filename();

    std::string& get_current_file();
};