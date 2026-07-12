#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    std::string initial_files[] = {"murder", "drones"};
    sm.init(true, initial_files);
    sm.change_file("murder");
    sm.write("number", 5);
    sm.commit();

    return 0;
}