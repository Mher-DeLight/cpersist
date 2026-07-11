#include <cpersist.h>
#include <iostream>
#include <sstream>
SaveManager sm;

class myotherclass {
public:
    int othernumber = 0;

    void serialize(const std::string& parent) {
        sm.write("othernumber", othernumber, parent);
    }
    void deserialize(const std::string& parent) {
        othernumber = sm.read<int>("othernumber", parent);
    }
};

class myclass {
public:
    int number = 0;
    myotherclass otherobj;

    void serialize(const std::string& parent) {
        sm.write("number", number, parent);
        sm.write("other_object", otherobj, parent);
    }
    void deserialize(const std::string& parent) {
        number = sm.read<int>("number", parent);
        otherobj = sm.read<myotherclass>("other_object", parent);
    }
};

int main()
{
    sm.change_file_safe("myfile");
    myclass obj;
    
    if (!sm.file_contains_data("object_instance")) {
        std::cout << "writing" << std::endl;
        sm.write("object_instance", obj);
        sm.commit(); 
    } else {
        std::cout << "reading" << std::endl;
        obj = sm.read<myclass>("object_instance");
        std::cout << "NUMBER: " << obj.number << "\nOTHER OBJECT MUMBER: " << obj.otherobj.othernumber << std::endl;
    }

    return 0;
}