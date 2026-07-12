#include <cpersist.h>
#include <iostream>

class myclass {
public:
    int number = 0;
    void serialize(const uint64_t& parent) {
        saveMgr.write("number", number, parent); // write number
    }
    void deserialize(const uint64_t& parent) {
        number = saveMgr.read<int>("number", parent); // load number
    }
};

int main()
{
    saveMgr.init(true);
    saveMgr.create_new_file("myfile");
    saveMgr.change_file("myfile");
    
    
    myclass obj;
    if (!saveMgr.file_exists("myfile") || !saveMgr.file_contains_data("obj_inst")) { // if no file is present, write it.
        std::cout << "Wrote: " << obj.number << std::endl;
        saveMgr.write("obj_inst", obj); // this will call the object's serialize() function
        saveMgr.commit();
    } else { // if a file is present, read it and output the current value
        std::cout << "Reading!" << std::endl;
        obj = saveMgr.read<myclass>("obj_inst"); // read as myclass
        std::cout << "Read: " << obj.number << std::endl;
    }


    return 0;
}