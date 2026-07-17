#include <cpersist.h>
#include <iostream>

class myclass {
public:
    int number = 0;

    void serialize(const std::string& parent) {
        saveMgr.write("number", number, parent);
    }
    void deserialize(const std::string& parent) {
        number = saveMgr.read<int>("number", std::nullopt, parent);
    }
};

int main()
{
    saveMgr.init(true);
    saveMgr.create_new_file("myfile");
    saveMgr.change_file("myfile");
    
    myclass obj;

    if (!saveMgr.file_exists("myfile") || !saveMgr.file_contains_data("inst")) { // if no file is present, write it.
        std::cout << "Writing!" << std::endl;
        saveMgr.write("inst", obj); // this will call the object's serialize() function
        std::cout << "Wrote: " << obj.number << std::endl;
        saveMgr.commit();
    } else { // if a file is present, read it and output the current value
        std::cout << "Reading!" << std::endl;
        obj = saveMgr.read<myclass>("inst"); // read as myclass
        std::cout << "Read: " << obj.number << std::endl;
    }


    return 0;
}