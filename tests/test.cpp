#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    std::string initial_files[] = {"murder", "drones"};
    sm.init(true, initial_files);

    sm.log_filenames();

    return 0;
}