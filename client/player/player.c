//
// Created by wlodi on 24.03.2023.
//

#include <malloc.h>
#include <sys/wait.h>
#include "player.h"

//this only use in add player
Player* create_player(int id) {
    Player* new_player = (Player*)malloc(sizeof(Player));
    new_player->x = id;
    new_player->y = id;
    new_player->id = id;
    new_player->next = new_player; // Initially, the player points to itself
    return new_player;
}

Player* add_player(Player* last_player, int id) {
    Player* new_player = create_player(id);
    if (last_player == NULL) {
        return new_player;
    }
    new_player->id = last_player->id + 1;
    new_player->next = last_player->next;
    last_player->next = new_player;
    return new_player;
}


int add_player_to_shared_memory(Player * player, Game * shared_memory){
    if(shared_memory != NULL && player != NULL){
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(shared_memory->players[i].isAssigned == FALSE){
                shared_memory->players[i] = *player;
                shared_memory->players[i].isAssigned = TRUE;
                return i;
            }
        }
        fprintf(stderr, "Error: Maximum number of players reached.\n");
        return -1;
    }
    return -1;
}


Player * get_last_player(Game * shared_memory){
    if(shared_memory != NULL){
        Player * last_player = NULL;
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(shared_memory->players[i].isAssigned == TRUE){
                last_player = &shared_memory->players[i];
            }
        }
        return last_player;
    }
    return NULL;
}

void play_game(Player* current_player, int num_rounds) {
    for (int i = 0; i < num_rounds; i++) {
        pid_t pid = fork();

        if (pid == 0) { // Child process (player)
            int shmid;
            Game* game = connect_to_shared_memory(&shmid);

            // Player's turn, interact with the shared memory
            printf("Player %d's turn\n", current_player->id);
            // ...

            // Detach the shared memory segment from the child process
            shmdt(game);

            exit(0);
        } else if (pid > 0) { // Parent process (game manager)
            // Wait for the child process (player) to finish their turn
            wait(NULL);

            // Move to the next player
            current_player = current_player->next;
        } else {
            perror("fork");
            exit(1);
        }
    }
}



void draw_map(){
    for (int y = 0; y < MAP_HEIGHT; y++){
            for(int x = 0; x < MAP_WIDTH; x++){
                char tile = map[y][x];
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
    curs_set(0);

    // Load and display the map
    load_map("/mnt/c/Users/wlodi/CLionProjects/SO2/mapEmpty.txt");
    draw_map();
    Game * shared_game_memory = NULL;
    int memory_id;
    shared_game_memory = connect_to_shared_memory(&memory_id);

    Player * last_player = get_last_player(shared_game_memory);

    if (last_player == NULL) {
        // If there are no players in the shared memory, create the first player with id 0
        add_player_to_shared_memory(create_player(0), shared_game_memory);
    } else {
        add_player_to_shared_memory(add_player(last_player, last_player->id + 1), shared_game_memory);
    }

    while(getch()){
        update_map(shared_game_memory);
        draw_map();
        refresh();
    }

    // Clean up and close ncurses
    endwin();
}
