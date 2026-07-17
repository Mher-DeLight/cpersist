#include <cpersist.h>
#include <iostream>

class myclass {
public:
    int number = 0;

    template<typename Archive>
    void archive(Archive& ar) {
        ar("number", number);
    }
};

int main()
{
    saveMgr.ensure_exists({"myfile", "yourfile"});
    saveMgr.open("myfile");
    
    myclass obj;

    if (!saveMgr.file_exists("myfile") || !saveMgr.contains("inst")) { // if no file is present, write it.
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