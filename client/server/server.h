//
// Created by wlodi on 26.03.2023.
//

#ifndef SO2_SERVER_H
#define SO2_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include "../player/player.h"
#include "../constants/constants.h"
#include <semaphore.h>

int create_and_attach_shared_memory(Game** game);
void initialize_game_state(Game* game);
void process_client_requests(Game* game);
void detach_and_remove_shared_memory(int shmid, Game* game);
void run_server();
int update_map(Game * game);
Game* connect_to_shared_memory(int* shmid);
int initialize_semaphore();
void destroy_semaphore();
int process_player_move_request(Game * game);
bool is_move_allowed(Game * game);
char get_next_move_char(Game * game);

#endif //SO2_SERVER_H
