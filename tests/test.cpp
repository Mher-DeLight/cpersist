#include <cpersist.h>
#include <iostream>

class myclass {
public:
    int number = 0;

    template<typename Archive>
    void archive(Archive& ar) {
        ar("number", number);
    }
};

int main() {
    saveMgr.enable_encryption(true);
    saveMgr.set_encryption_key("mysecretkey");

    saveMgr.open("myfile");

    myclass obj;

    saveMgr.sync("inst", obj);
    saveMgr.commit();
    
    std::cout << "number=" << obj.number << std::endl;
    


    return 0;
}