//
// Created by wlodi on 24.03.2023.
//

#include <malloc.h>
#include <sys/wait.h>
#include "player.h"


int send_create_player_request(Game * game){
    if(game == NULL)
        return -1;

    game->create_player = true;

    return 0;
}

int move_player(Game * game, int ch, Player * current_player){

    if(game == NULL){
        return printf("Game structure null - probably not initialized"), -1;
    }
    if(current_player == NULL){
        return printf("Current player is invalid - null"), -1;
    }


    /// ask server to move the current player
    switch (ch) {
        case KEY_UP:
            game->next_move = UP;
            break;
        case KEY_DOWN:
            // Move player down
            game->next_move = DOWN;
            break;
        case KEY_LEFT:
            // Move player left
            game->next_move = LEFT;
            break;
        case KEY_RIGHT:
            // Move player right
            game->next_move = RIGHT;
            break;
        default:
            game->next_move = NONE;

    }
    /// give the server info about current player
    game->current_player = current_player;

    return 0;
}


void draw_map(Game * game){
    for (int y = 0; y < MAP_HEIGHT; y++){
            for(int x = 0; x < MAP_WIDTH; x++){
                char tile = game->map[y][x];
                if (tile == 'W'){
                    mvaddch(y, x, '#' | A_BOLD | COLOR_PAIR(1));
                } else {
                    mvaddch(y, x, tile);
                }
            }
    }
}

void run_client() {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    Game * shared_game_memory = NULL;
    int memory_id;
    shared_game_memory = connect_to_shared_memory(&memory_id);

    ///Send create player request
    send_create_player_request(shared_game_memory);

    //created the player
    Player * current_player = NULL;
    while(true){
        if(shared_game_memory->created == true && shared_game_memory->create_player == false){
            current_player = shared_game_memory->current_player;
            break;
        }
    }


    //client loop
    int ch;
    while (true) { // Exit the loop when the 'q' key is pressed
        if((ch = getch()) == 'q')
            break;
        draw_map(shared_game_memory);
        move_player(shared_game_memory ,ch, current_player);
        // Update the screen, draw player, etc
        refresh();

    }

    // Clean up and close ncurses
    endwin();
}
