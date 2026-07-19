#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("mysecretkey");

    saveMgr.open("myfile");

    int a = 1;
    int b = 2;
    int c = 3;

    saveMgr.sync("a", a);
    saveMgr.sync("b", b);
    saveMgr.sync("c", c);
    saveMgr.commit();
    
    std::cout << "a=" << a << " b=" << b << " c=" << c << std::endl;
    


    return 0;
}