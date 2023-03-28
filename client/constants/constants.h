//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_CONSTANTS_H
#define SO2_CONSTANTS_H

#include <stdbool.h>
#define MAP_WIDTH 50
#define MAP_HEIGHT 21
#define MAX_PLAYERS 4
#define SHM_KEY 1234
#define SHM_SIZE 2048
#define READY 1
#define MOVED 2
#define GAME_END 0

typedef struct Player {
    int id;
    int x;
    int y;
    bool isAssigned;
    // Add other player attributes here, such as name, score, etc.
    struct Player* next;
} Player;

typedef struct {
    int game_status;
    int server_status;
    Player players[MAX_PLAYERS];
} Game;


extern char map[MAP_HEIGHT][MAP_WIDTH];
void load_map(char * filename);

#endif //SO2_CONSTANTS_H
