//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_CONSTANTS_H
#define SO2_CONSTANTS_H

#include <stdbool.h>
#include <semaphore.h>

#define MAP_WIDTH 50
#define MAP_HEIGHT 21
#define MAX_PLAYERS 1
#define SHM_KEY 1249
#define SHM_SIZE 2048
#define READY 1
#define MOVED 2
#define GAME_END 0
#define COIN 1
#define TREASURE 10
#define CHEST 50
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

typedef struct Treasure{
    char type;
    int amount;
    int x;
    int y;
} Treasure;

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
    int carried_coins;
    int deaths;
    int brought_coins;
    pid_t client_pid;
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

typedef struct CoinDrop{
    int value;
    int x;
    int y;
    bool used;

}CoinDrop;

typedef struct {
    int game_status;
    pid_t server_pid;
    bool respawn_beast;
    bool respawn_coins;
    int server_status;
    Player players[MAX_PLAYERS];
    Beast beast;
    char map[MAP_HEIGHT][MAP_WIDTH];
    int number_of_players;
    sem_t sem;
    sem_t sem_draw;
    pthread_mutex_t mutex;
    pthread_cond_t beast_cond;
    pthread_cond_t game_end_cond;
    Treasure treasure[10];
    CoinDrop drops[5];
    int campsite_x;
    int campsite_y;
} Game;

void load_map(const char * filename, Game * game);

#endif //SO2_CONSTANTS_H
