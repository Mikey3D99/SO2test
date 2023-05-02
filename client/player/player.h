//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_PLAYER_H
#define SO2_PLAYER_H
#include "../server/server.h"
#include "ncurses.h"
#include "../constants/constants.h"
void *redraw_map_thread(void *data);
void run_client();
int move_player(int ch, Player * player);
void print_map_debug_client();
#endif //SO2_PLAYER_H
