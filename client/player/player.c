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
void draw_fov_map(Player *player) {
    int FOV_HEIGHT = 5;
    int FOV_WIDTH = 5;

    for (int y = 0; y < FOV_HEIGHT; y++) {
        for (int x = 0; x < FOV_WIDTH; x++) {
            char tile = player->fov_map[y][x];
            if (tile == 'W') {
                mvaddch(y, x, '#' | A_BOLD | COLOR_PAIR(1));
            } else {
                mvaddch(y, x, tile);
            }
        }
    }

    // Display player information below the map
    mvprintw(FOV_HEIGHT + 1, 0, "Coins: %03d", player->carried_coins);
    mvprintw(FOV_HEIGHT + 2, 0, "Brought: %03d", player->brought_coins);
    mvprintw(FOV_HEIGHT + 3, 0, "Deaths: %02d", player->deaths);
    mvprintw(FOV_HEIGHT + 4, 0, "X: %02d", player->x);
    mvprintw(FOV_HEIGHT + 5, 0, "Y: %02d", player->y);
}


typedef struct GameAndPlayer {
    Game *game;
    Player *player;
} GameAndPlayer;


void *redraw_map_thread_client(void *data) {
    GameAndPlayer *game_and_player = (GameAndPlayer *)data;
    Game *game = game_and_player->game;
    Player *player = game_and_player->player;
    while (true) {
        if (sem_wait(&game->sem_draw) == -1) {
            perror("sem_wait");
            break;
        }
        //update_map(game);
        //update_fov(game);
        draw_fov_map(player);
        if (sem_post(&game->sem_draw) == -1) {
            perror("sem_post");
            break;
        }
        //print_map_debug_client();
        //erase();
        refresh();// Redraw every 100ms, adjust this value as needed
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

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);

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
        return;    }

    GameAndPlayer game_and_player;
    game_and_player.game = shared_game_memory;
    game_and_player.player = &shared_game_memory->players[player_id];

    //printf("po wyslaniu rq");
    pthread_t redraw_thread;
    pthread_create(&redraw_thread, NULL, redraw_map_thread_client, &game_and_player);


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
    }

    // Clean up and exit
    pthread_cancel(redraw_thread);
    pthread_join(redraw_thread, NULL);
    endwin();
}
