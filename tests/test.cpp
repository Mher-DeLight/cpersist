#include <iostream>
#include <cpersist.h>

int main () {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("@a^*1#5a_h");

    saveMgr.open("myfile");
    saveMgr.write("number", 3);
    saveMgr.commit();
}