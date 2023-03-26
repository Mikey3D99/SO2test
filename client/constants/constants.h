//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_CONSTANTS_H
#define SO2_CONSTANTS_H
#define MAP_WIDTH 50
#define MAP_HEIGHT 21

extern char map[MAP_HEIGHT][MAP_WIDTH];
void load_map(char * filename);
void draw_map();

#endif //SO2_CONSTANTS_H
