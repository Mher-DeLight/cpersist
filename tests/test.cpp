#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("hellothere");

    saveMgr.open("myfile");
    saveMgr.write("number", 1);
    saveMgr.write("other_number", 2);
    saveMgr.commit();

    std::cout << "Number: " << saveMgr.read<int>("number") << " Other number: " << saveMgr.read<int>("other_number") << std::endl;

    return 0;
}