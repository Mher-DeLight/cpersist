#include <cpersist.h>
#include <iostream>

int main()
{
    SaveManager sm;
    sm.change_file_safe("myfile");
    sm.write("magic_number", 6);
    sm.write("the_letter_h", "h");
    sm.commit();

    return 0;
}