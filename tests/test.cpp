#include <iostream>
#include <cpersist.h>

int main () {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("wowyoureallyfoundit");
}