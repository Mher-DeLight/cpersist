#include <cpersist.h>
#include <iostream>
#include <sstream>
SaveManager sm;


class myclass {
public:
    int number = 2;

    void serialize(const std::string& parent) {
        sm.write("number", number, parent);
    }
    void deserialize(const std::string& parent) {
        number = sm.read<int>("number", parent);
    }
};

int main()
{
    sm.change_file_safe("myfile");
    myclass obj;
    obj = sm.read<myclass>("object_instance");
    std::cout << obj.number << std::endl;
    // sm.write("object_instance", obj);
    // sm.commit(); 

    return 0;
}