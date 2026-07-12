#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    std::string initial_files[] = {"murder", "drones"};
    sm.init(true, initial_files);
    sm.change_file("murder");
    // sm.write("other_number", 15);
    // sm.commit();
    std::cout << sm.read<int>("other_number") << std::endl;

    return 0;
}