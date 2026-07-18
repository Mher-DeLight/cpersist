#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("mysecretkey");

    saveMgr.open("myfile");

    saveMgr.write("a", 3);
    saveMgr.write("b", 5);
    saveMgr.commit();

    std::cout << "a=" << saveMgr.read<int>("a") << " b=" << saveMgr.read<int>("b") << std::endl;

    return 0;
}