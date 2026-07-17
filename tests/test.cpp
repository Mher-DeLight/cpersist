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

int main()
{
    saveMgr.open("myfile");
    saveMgr.write("inst", myclass());
    saveMgr.commit();
    std::cout << saveMgr.contains("inst") << std::endl;


    return 0;
}