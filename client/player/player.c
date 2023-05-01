//
// Created by wlodi on 24.03.2023.
//
#include <malloc.h>
#include <pthread.h>
#include "player.h"


int send_create_player_request(Game * game){
    if(game == NULL)
        return -1;

    // Wait for the semaphore
    if (sem_wait(&game->sem) == -1) {
        perror("sem_wait");
        return -1;
    }

    int id = game->number_of_players;
    game->players[id].isAssigned = true;


    // Release the semaphore
    if (sem_post(&game->sem) == -1) {
        perror("sem_post");
        return -1;
    }

    return id;
}


/// ten segment zmienia tylko indywidualne pola playera, czyli nie powinno byc zadnych konfliktow
int move_player(int ch, Player * player){

    if(player == NULL){
        return printf("Current player is invalid - null"), -1;
    }

    /// ask server to move the current player
    switch (ch) {
        case KEY_UP:
            player->next_move = UP;
            break;
        case KEY_DOWN:
            // Move player down
            player->next_move = DOWN;
            break;
        case KEY_LEFT:
            // Move player left
            player->next_move = LEFT;
            break;
        case KEY_RIGHT:
            // Move player right
            player->next_move = RIGHT;
            break;
        default:
            player->next_move = NONE;

    }
    return 0;
}


void draw_map(Game * game){
    for (int y = 0; y < MAP_HEIGHT; y++){
            for(int x = 0; x < MAP_WIDTH - 1; x++){
                char tile = game->map[y][x];
                if (tile == 'W'){
                    mvaddch(y, x, '#' | A_BOLD | COLOR_PAIR(1));
                } else {
                    mvaddch(y, x, tile);
                }
            }
    }
}


void *redraw_map_thread(void *data) {
    Game *shared_data = (Game *)data;
    while (true) {
        if (sem_wait(&shared_data->sem) == -1) {
            perror("sem_wait");
            break;
        }
        draw_map(shared_data);
        // Release the semaphore
        if (sem_post(&shared_data->sem) == -1) {
            perror("sem_post");
            break;
        }
        refresh();
        // Redraw every 100ms, adjust this value as needed
    }
}


//TODO: dodaj semafor zeby przy zapisie do pamieci dzielonej (same wyslanie sygnalu) bylo zablokowane dla innych
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
    if(shared_game_memory == NULL){
        printf("error memri");
        return;
    }

    ///Send create player request
    int player_id = send_create_player_request(shared_game_memory);
    if(player_id < 0){
        printf("error creating a player");
        return;
    }

    //printf("po wyslaniu rq");
    pthread_t redraw_thread;
    pthread_create(&redraw_thread, NULL, redraw_map_thread, shared_game_memory);

    //client loop
    int ch;
    while (true) { // Exit the loop when the 'q' key is pressed
        ch = getch();
        if (ch == 'q') {
            break;
        }
        // Wait for the semaphore
        if (sem_wait(&shared_game_memory->sem) == -1) {
            perror("sem_wait");
            break;
        }
        if(&shared_game_memory->players[player_id] != NULL ){
            move_player(ch, &shared_game_memory->players[player_id]);
        }
        // Release the semaphore
        if (sem_post(&shared_game_memory->sem) == -1) {
            perror("sem_post");
            break;
        }
        // Update the screen, draw player, etc
        refresh();
    }

    // Clean up and exit
    pthread_cancel(redraw_thread);
    pthread_join(redraw_thread, NULL);
    endwin();
}
