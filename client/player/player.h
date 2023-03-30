//
// Created by wlodi on 24.03.2023.
//

#ifndef SO2_PLAYER_H
#define SO2_PLAYER_H
#include "../server/server.h"
#include "ncurses.h"
#include "../constants/constants.h"


void draw_map(Game * game);
Player* create_player(int id);
Player* add_player(Player* last_player, int id);
void play_game(Player* current_player, int num_rounds);
void run_client();
int move_player(Game * game, int ch, Player * current_player);
#endif //SO2_PLAYER_H
