#include <iostream>
#include <cpersist.h>

int main () {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("wowyoureallyfoundit");

    std::vector<int> myvec = {3, 5, 2 , 1};

    saveMgr.open("myfile");
    saveMgr.write("vec", myvec);
    saveMgr.commit();
    for (auto el : saveMgr.read<std::vector<int>>("vec")) {
        std::cout << el << std::endl;
    }
}