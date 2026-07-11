#include <cpersist.h>
#include <iostream>
#include <sstream>

int main()
{
    SaveManager sm;
    int high_score = 3;
    sm.create_new_file("scores");
    sm.change_file("scores");
    if (!sm.file_contains_data("highscore")) {
        sm.write("highscore", high_score); // save if not saved already
        sm.commit();
    } else {
        high_score = sm.read<int>("highscore");
    }

    return 0;
}