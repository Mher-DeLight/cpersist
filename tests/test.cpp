#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("mysecretkey");

    saveMgr.open("myfile2");

    if (!saveMgr.file_exists_on_disk("myfile") || !saveMgr.contains({"a", "b", "c"})) {
        saveMgr.write("a", 3);
        saveMgr.write("b", 5);
        saveMgr.write("c", 6);
        saveMgr.commit();
    } else {
        std::cout << "c=" << saveMgr.read<int>("c") << " b=" << saveMgr.read<int>("b") << std::endl;
    }
    

    return 0;
}