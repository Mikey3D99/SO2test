//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_CONSTANTS_H
#define SO2_CONSTANTS_H

#include <stdbool.h>
#include <semaphore.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 21
#define MAX_PLAYERS 2
#define SHM_KEY 1234
#define SHM_SIZE 2048
#define READY 1
#define MOVED 2
#define GAME_END 0

typedef enum {
    NONE,
    UP,
    DOWN,
    LEFT,
    RIGHT
} MoveDirection;

typedef struct {
    int x;
    int y;
} Point;


typedef struct Player {
    int id;
    int x;
    int y;
    int spawn_x;
    int spawn_y;
    bool isAssigned;
    // Add other player attributes here, such as name, score, etc.
    MoveDirection next_move;
    bool is_alive;
    char fov_map[5][5];
} Player;

typedef struct Beast {
    int id;
    Point current_pos;
    int spawn_x;
    int spawn_y;
    bool isAssigned;
    MoveDirection next_move;
    bool allow_move;
} Beast;


typedef struct {
    int game_status;
    int server_status;
    Player players[MAX_PLAYERS];
    Beast beast;
    char map[MAP_HEIGHT][MAP_WIDTH];
    int number_of_players;
    sem_t sem;
    sem_t sem_draw;
    pthread_mutex_t mutex;
    pthread_cond_t beast_cond;
} Game;

void load_map(const char * filename, Game * game);

#endif //SO2_CONSTANTS_H
