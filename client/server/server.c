//
// Created by wlodi on 26.03.2023.
//
#include "server.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

// 1 pshared - share semaphore among processes; 1 - initial value
int initialize_semaphore(Game* game) {
    if (sem_init(&game->sem, 1, 1) == -1) {
        perror("sem_init");
        return -1;
    }
    return 0;
}


void destroy_semaphore(Game* game) {
    if (sem_destroy(&game->sem) == -1) {
        perror("sem_destroy");
    }
}


int create_and_attach_shared_memory(Game** game) {
    int shmid = shmget(SHM_KEY, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return -1;
    }

    *game = (Game*)shmat(shmid, NULL, 0);
    if (*game == (Game*)-1) {
        perror("shmat");
        return -1;
    }

    return shmid;
}

void random_coordinates(int *x, int *y) {
    *x = (rand() %(MAP_WIDTH - 4 + 1)) + 4;
    *y = (rand() %(MAP_HEIGHT - 4 + 1)) + 4;
}

bool is_valid_position(Game* game, int x, int y) {
    if (game->map[y][x] != ' ') {
        return false;
    }

    // tutaj ma sprawdzic czy pozycja x y nie jest juz zabrana przez jakiegos gracza
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (game->players[i].x == x && game->players[i].y == y) {
            return false;
        }
    }

    return true;
}

int initialize_beasts(Game * game){
    if(game == NULL){
        return -1;
    }

    srand(time(0)); // Seed the random number generator

    int x, y;

    do {
        random_coordinates(&x, &y);
    } while (!is_valid_position(game, x, y));

    game->beast.id = 0;
    game->beast.current_pos.x = x;
    game->beast.current_pos.y = y;
    game->beast.spawn_x = x;
    game->beast.spawn_y = y;
    game->beast.isAssigned = true;
    game->beast.next_move = NONE;
    game->beast.allow_move = false;

    return 0;
}


void init_players(Game* game) {
    if (game == NULL) {
        return;
    }

    srand(time(0)); // Seed the random number generator

    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->players[i].id = i;
        int x, y;

        do {
            random_coordinates(&x, &y);
        } while (!is_valid_position(game, x, y));

        game->players[i].x = x;
        game->players[i].y = y;
        game->players[i].spawn_x = x;
        game->players[i].spawn_y = y;
        game->players[i].isAssigned = false;
        game->players[i].next_move = NONE;
    }
}



void initialize_game_state(Game* game, const char * filename) {

    if(game != NULL && filename != NULL){
        game->game_status = READY; // Example status: game is ready
        game->server_status = READY;
        game->number_of_players = 0;
        memset(game->map, 'a', MAP_HEIGHT * MAP_WIDTH);
        load_map(filename, game);

        for(int i = 0; i < MAP_HEIGHT; i++) {
            game->map[i][MAP_WIDTH - 1] = '\0';
        }

        // AFTER LOADING MAP
        pthread_mutex_init(&game->mutex, NULL);
        pthread_cond_init(&game->beast_cond, NULL);
        init_players(game);
        initialize_beasts(game);
        return;
    }
    printf("error with initialization!");
}


char get_next_move_char(Game * game, Player * player){
    char for_return = 'W';

    if(game != NULL && player != NULL && player->isAssigned){
        int x = player->x;
        int y = player->y;
        //printf("[%d]", player->next_move);
        switch(player->next_move){
            case UP:
                for_return = game->map[y - 1][x];
                break;
            case DOWN:
                for_return = game->map[y + 1][x];
                break;
            case RIGHT:
                for_return = game->map[y][x + 1];
                break;
            case LEFT:
                for_return = game->map[y][x - 1];
                break;
            case NONE:
                break;
        }
    }

    return for_return;
}

bool is_move_allowed(Game * game, Player * player){
    if(game != NULL && player != NULL && player->isAssigned){
        Player *shared_player = &game->players[player->id];

        char obstacles[] = "W#";
        char next_move = get_next_move_char(game, shared_player);
        //printf("[%c]",  next_move);

        // check if the next move is one of the obstacles
        if (strchr(obstacles, next_move) != NULL) {
            return false;
        }
        else {
            return true;
        }
    }
    return false;
}



int process_player_move_request(Game * game){
    if(game != NULL){

        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == true){

                if(is_move_allowed(game, &game->players[i])){
                    game->map[game->players[i].y][game->players[i].x] = ' ';

                    switch(game->players[i].next_move){
                        case UP:
                            game->players[i].y--;
                            break;
                        case DOWN:
                            game->players[i].y++;
                            break;
                        case RIGHT:
                            game->players[i].x++;
                            break;
                        case LEFT:
                            game->players[i].x--;
                            break;
                        case NONE:
                            break;
                    }

                    // next move clear
                    game->players[i].next_move = NONE;

                    return 0;
                }
            }
        }
    }
    return -1;
}


Point* bresenham(int x1, int y1, int x2, int y2, int *num_points) { // bresenham algorithm for checking the indices between player and the enemy
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;
    int e2;

    // Calculate an upper bound for the number of points
    int max_points = (dx > dy ? dx : dy) + 1;
    Point *points = (Point *)malloc(max_points * sizeof(Point));
    *num_points = 0;

    while (1) {
        Point p = {x1, y1};
        points[(*num_points)++] = p;
        if (x1 == x2 && y1 == y2) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y1 += sy;
        }
    }

    return points;
}

bool is_player_obstructed_by_wall(int player_x, int player_y, int beast_x, int beast_y, Game * game, int range){
    if(game == NULL){
        return true;
    }
    int num_points = 0;
    Point * points_between = bresenham(beast_x, beast_y, player_x, player_y, &num_points);

    if(num_points > range){ //if out of sight
        free(points_between);
        return true;
    }

    if(points_between != NULL){
        for (int i = 0; i < num_points; i++) {
            //printf("[%c]\n", game->map[points_between[i].y][points_between[i].x]);
            if (game->map[points_between[i].y][points_between[i].x] == 'W') {
                free(points_between);
                return true;
            }
        }
    }
    free(points_between);
    return false;
}

int find_closest_player_id(Game * game){
    if(game == NULL){
        return -1;
    }

    int point_number = 0;
    Player * min_player = NULL;
    int min_distance = 0;

    for(int i = 0; i < MAX_PLAYERS; i++){
        if(game->players[i].isAssigned){
            Point * points = bresenham(game->players[i].x, game->players[i].y,
                                       game->beast.current_pos.x, game->beast.current_pos.y,
                                       &point_number);
            free(points);
            if(i == 0){
                min_distance = point_number;
                min_player = &game->players[i];
            }
            else{
                if(min_distance > point_number){
                    min_distance = point_number;
                    min_player = &game->players[i];
                }
            }
        }
    }

    if (min_player == NULL) {
        return -1; // Or another suitable error value.
    }
    return min_player->id;
}



// byc moze tutaj osobny watek aby bestia patrzyla caly czas czy jakis gracz nie wszedl w jej sight
void beast_follow_player(int player_id, Game * game){
    if(game == NULL){
        return;
    }

    int distanceX = abs(game->players[player_id].x - game->beast.current_pos.x);
    int distanceY = abs(game->players[player_id].y - game->beast.current_pos.y);
    // no walls and within range
    if(!is_player_obstructed_by_wall(game->players[player_id].x,
                                     game->players[player_id].y,
                                     game->beast.current_pos.x,
                                     game->beast.current_pos.y, game, 7)){

        printf("[BEAST][%d]\n", player_id);
        // clean the spot
        game->map[game->beast.current_pos.y][game->beast.current_pos.x] = ' ';

        int beast_x = game->beast.current_pos.x;
        int beast_y = game->beast.current_pos.y;

        // Before changing the beast's position, check if there's no wall
        if (distanceX > distanceY) {
            beast_x += (game->players[player_id].x > game->beast.current_pos.x) ? 1 : -1;
        } else {
            beast_y += (game->players[player_id].y > game->beast.current_pos.y) ? 1 : -1;
        }
        if(game->map[beast_y][beast_x] != 'W'){
            game->beast.current_pos.x = beast_x;
            game->beast.current_pos.y = beast_y;
        }
        if(game->beast.current_pos.x == game->players[player_id].x &&
                game->beast.current_pos.y == game->players[player_id].y){
            game->players[player_id].x =  game->players[player_id].spawn_x;
            game->players[player_id].y =  game->players[player_id].spawn_y;
        }
    }
}


typedef struct {
    Game *game;
    int beast_id;
} BeastThreadArg;

void *beast_thread_func(void *arg) {
    BeastThreadArg *thread_arg = (BeastThreadArg *)arg;
    Game *game = thread_arg->game;

    while(game->server_status){
        // Wait for the semaphore
        if (sem_wait(&game->sem) == -1) {
            perror("sem_wait");
            free(arg);
            return NULL;
        }

        if(game->number_of_players > 0){
            int closest_player_id = find_closest_player_id(game);
            beast_follow_player(closest_player_id, game);
        }

        // Release the semaphore
        if (sem_post(&game->sem) == -1) {
            perror("sem_post");
        }
    }
    free(arg);
    return NULL;
}

void process_beasts(Game *game) {
    if (game == NULL) {
        return;
    }
}



void listen_to_client_connections(Game* game) {
    while(true){
        if (game->server_status == READY && game != NULL){
            fflush(stdout);

            if (sem_wait(&game->sem) == -1) {
                perror("sem_wait");
                break;
            }

            // client sent a request for creating a player. The server has to constantly look for this request.
            for(int i = game->number_of_players; i < MAX_PLAYERS; i++){
                if(game->players[i].isAssigned == true){
                    game->number_of_players++; // increment player count

                    //initialize new player
                    game->players[i].id = i;
                }
            }

            // Release the semaphore
            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }
        }
    }
}


void detach_and_remove_shared_memory(int shmid, Game* game) {
    if (shmdt(game) == -1) {
        perror("shmdt");
    }

    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        perror("shmctl");
    }
}

///TODO: delete fixed offset - players should spawn randomly
int update_map(Game * game){
    if(game != NULL){
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == TRUE){
                game->map[game->players[i].y][game->players[i].x] = (char)('0' + game->players[i].id);
            }
        }
        // Update the single beast's position on the map
        if(game->beast.isAssigned == TRUE){
            game->map[game->beast.current_pos.y][game->beast.current_pos.x] = '*';
        }
        return 0;
    }
    return -1;
}



Game* connect_to_shared_memory(int* shmid) {
    *shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (*shmid < 0) {
        perror("shmget");
        exit(1);
    }

    Game* game = (Game*)shmat(*shmid, NULL, 0);
    if (game == (Game*)-1) {
        perror("shmat");
        exit(1);
    }

    return game;
}

///TODO: dodaj wątek do kazdego nowego dolaczajcego
/// klienta aby serwer mogl updatowac mape w jednej turze
/// dla wszystkich klientow

void clear_positions_on_map(Game * game){
    if(game != NULL){
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == TRUE){
                game->map[game->players[i].y][game->players[i].x] = ' ';
            }
        }
        // Clear the single beast's position on the map
        if(game->beast.isAssigned == TRUE){
            game->map[game->beast.current_pos.y][game->beast.current_pos.x] = ' ';
        }
    }
}

void print_map_debug(Game *game) {
    if (game != NULL) {
        for (int i = 0; i < MAP_HEIGHT; i++) {
            printf("%.*s\n", MAP_WIDTH, game->map[i]);
        }
    }
}

void *beast_behavior_thread(void *data) {
    Game *game = (Game *)data;

    while (1) {
        if (pthread_mutex_lock(&game->mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }

        pthread_cond_wait(&game->beast_cond, &game->mutex);

        if (sem_wait(&game->sem) == -1) {
            perror("sem_wait");
            pthread_mutex_unlock(&game->mutex);
            break;
        }

        beast_follow_player(find_closest_player_id(game), game);

        if (sem_post(&game->sem) == -1) {
            perror("sem_post");
            pthread_mutex_unlock(&game->mutex);
            break;
        }

        if (pthread_mutex_unlock(&game->mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }
    }

    return NULL;
}




void run_server() {
    Game* game;

    // Initialize ncurses
    //initscr();
   // cbreak();
    //noecho();
    //keypad(stdscr, TRUE);
    //curs_set(0);
    // Initialize the mutex


    int shmid = create_and_attach_shared_memory(&game);
    if (shmid < 0) {
        exit(1);
    }
    initialize_semaphore(game);

    if (sem_wait(&game->sem) == -1) {
        perror("sem_wait");
        return;
    }

    initialize_game_state(game, "/mnt/c/Users/wlodi/CLionProjects/SO2/mapEmpty.txt");

    if (sem_post(&game->sem) == -1) {
        perror("sem_post");
        return;
    }

    pthread_t client_requests_thread;
    int err = pthread_create(&client_requests_thread, NULL, (void *) listen_to_client_connections, game);
    if (err != 0) {
        perror("pthread_create: client_requests_thread");
        exit(1);
    }

    // Before entering the server loop
    pthread_t beast_thread;
    pthread_create(&beast_thread, NULL, beast_behavior_thread, game);

    //pthread_t redraw_thread;
   // err = pthread_create(&redraw_thread, NULL, redraw_map_thread, game);
   // if (err != 0) {
    //    perror("pthread_create: redraw_thread");
   //     exit(1);
   // }

    // Create beast threads
   // process_beasts(game);

    while (1) {
        if (sem_wait(&game->sem) == -1) {
            perror("sem_wait");
            break;
        }

        clear_positions_on_map(game);
        process_player_move_request(game);

        update_map(game);
        print_map_debug(game);

        if (sem_post(&game->sem) == -1) {
            perror("sem_post");
            break;
        }

        if (pthread_mutex_lock(&game->mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }

        pthread_cond_signal(&game->beast_cond);

        if (pthread_mutex_unlock(&game->mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }

        sleep(1);
    }

    //pthread_cancel(redraw_thread);
   // pthread_join(redraw_thread, NULL);

    // Before exiting the server
    pthread_cancel(beast_thread);
    pthread_join(beast_thread, NULL);
    pthread_join(client_requests_thread, NULL);
    game->server_status = 0;
    detach_and_remove_shared_memory(shmid, game);
    destroy_semaphore(game);
    // Destroy the mutex before the program exits
    pthread_mutex_destroy(&game->mutex);
    //endwin();
}