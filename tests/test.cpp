#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    
    sm.create_new_file("a");
    sm.change_file("a");
    if (!sm.file_contains_data("z")) {
        std::string x = "look at me still talking when there's science to do";
        sm.write("z", x);
        sm.commit();
    } else {
        std::cout << sm.read<std::string>("z") << std::endl;
    }

    return 0;
}