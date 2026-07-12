#include <cpersist.h>
#include <iostream>

SaveManager sm;
class myclass {
public:
    int number = 0;
    void serialize(const uint64_t& parent) {
        sm.write("number", number, parent);
    }
    void deserialize(const uint64_t& parent) {
        number = sm.read<int>("number", parent);
    }
};

int main()
{
    sm.init(true);
    sm.create_new_file("myfile");
    sm.change_file("myfile");
    
    myclass obj;
    if (!sm.file_exists("myfile") || !sm.file_contains_data("obj_inst")) {
        std::cout << "Wrote: " << obj.number << std::endl;
        sm.write("obj_inst", obj);
        sm.commit();
    } else {
        std::cout << "Reading!" << std::endl;
        obj = sm.read<myclass>("obj_inst");
        std::cout << "Read: " << obj.number << std::endl;
    }


    return 0;
}