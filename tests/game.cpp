#include <cpersist.h>
#include <cstdlib>
#include <iostream>
#include <termios.h>
#include <unistd.h>

char getch() {
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);

    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char c = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);

    return c;
}

int main() {
    auto file = cpersist::File("player");
    file.enable_autocommit_on_destroy(true);

    int x = 10;
    int y = 10;
    int speed = 1;

    file.sync("x", x);
    file.sync("y", y);

    while (true) {
        system("clear");

        for (int i = 0; i < y; i++) {
            std::cout << "\n";
        }

        for (int i = 0; i < x; i++) {
            std::cout << " ";
        }

        std::cout << "O";

        char key = getch();

        if (key == 'd')
            x += speed;
        else if (key == 'a')
            x -= speed;
        else if (key == 'w')
            y -= speed;
        else if (key == 's')
            y += speed;
        else if (key == 'q')
            break;

        if (x < 0)
            x = 0;

        if (y < 0)
            y = 0;
    }

    system("clear");

    file.write("x", x);
    file.write("y", y);
}