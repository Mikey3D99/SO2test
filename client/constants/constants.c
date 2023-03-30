//
// Created by wlodi on 24.03.2023.
//

#include "constants.h"
#include <stdio.h>
#include <errno.h>


void load_map(const char *filename, Game * game) {

    if(game == NULL){
        fprintf(stderr, "Error with game structure: %s (error %d)\n", filename, errno);
        return;
    }

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
            game->map[y][x] = (char)c;
        }
    }
}

