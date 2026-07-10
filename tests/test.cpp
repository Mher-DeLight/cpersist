#include <cpersist.h>
#include <iostream>

int main()
{
    SaveManager sm;
    sm.create_new_file("hello_world");
    sm.change_file("hello_world");
    sm.write(3);
    sm.commit();

    return 0;
}