#include <cpersist.h>
#include <iostream>

int main() {
    SaveManager sm;
    sm.create_new_file("hello_world");
    sm.create_new_file("bye_world");
    sm.log_filenames();

    sm.change_file_safe("i was at my job");
    sm.log_current_filename();

    sm.change_file("good job");
    sm.log_current_filename();

    return 0;
}