#include <cpersist.h>
#include <iostream>

int main()
{
    saveMgr.open("myfile");
    int number;
    saveMgr.read_into("number", number);
    std::cout << number << std::endl;
    saveMgr.commit();


    return 0;
}