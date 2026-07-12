#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    std::string initial_files[] = {"murder", "drones"};
    sm.init(true, initial_files);
    sm.change_file("murder");
    // sm.write("number", 3);
    // sm.commit();
    std::cout << sm.read<int>("number") << std::endl;

    return 0;
}