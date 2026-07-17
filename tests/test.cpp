#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.open("myfile");
    saveMgr.write("number", 3);
    saveMgr.commit();

    return 0;
}