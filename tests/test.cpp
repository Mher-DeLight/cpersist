#include <iostream>
#include <cpersist.h>


int main () {
    saveMgr.enable_encryption(false);

    saveMgr.open("myfile");
    int number = 0;
    saveMgr.link("number", number);
    number = 5;
    saveMgr.commit(); // should write 5
}