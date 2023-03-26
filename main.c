#include <ncurses.h>
#include "client/constants/constants.h"


int main()
{
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    load_map("/mnt/c/Users/wlodi/CLionProjects/SO2/mapEmpty.txt");
    draw_map();

    // Refresh screen and wait for key press
    refresh();
    getch();

    // Clean up
    endwin();
    return 0;
}