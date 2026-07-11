#include <cpersist.h>
#include <iostream>
#include <sstream>


class myclass {
public:
    int number = 5;

    void serialize(std::stringstream& s) {
        s << number;
    }
};

SaveManager sm;
int main()
{
    sm.change_file_safe("myfile");

    // myclass obj;
    sm.write("object_instance", 4);
    sm.commit(); 
    

    

    return 0;
}