#include <cpersist.h>
#include <iostream>

int main()
{
    SaveManager sm;
    sm.change_file_safe("myfile");
    int number = 3;
    sm.write("magic_number", number);
    sm.commit();

    return 0;
}