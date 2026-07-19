#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("mysecretkey");

    saveMgr.open("myfile");

    if (!saveMgr.file_exists_on_disk("myfile")) {
        saveMgr.write("b", 5);
        saveMgr.write("a", 3);
        saveMgr.commit();
    } else {
        std::cout << "a=" << saveMgr.read<int>("a") << " b=" << saveMgr.read<int>("b") << std::endl;
    }
    

    return 0;
}