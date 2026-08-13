#include <cpersist.h>
#include <iostream>

int main() {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("wowyoureallyfoundit");

    saveMgr.open("myfile");

    saveMgr.write("number", 3);
    saveMgr.commit();

    std::cout << saveMgr.read<int>("number") << std::endl;
}