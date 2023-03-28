//
// Created by wlodi on 24.03.2023.
//

#include "constants.h"
#include <stdio.h>
#include <errno.h>

char map[MAP_HEIGHT][MAP_WIDTH] = {};

void load_map(char *filename) {
    // Open map file
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error opening map file: %s (error %d)\n", filename, errno);
        return;
    }

    // Read map data from file
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            int c = fgetc(fp);
            if (c == EOF) {
                fprintf(stderr, "Error reading map file: %s (error %d)\n", filename, errno);
                fclose(fp);
                return;
            }
            map[y][x] = (char)c;
        }
    }
}

