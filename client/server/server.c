//
// Created by wlodi on 26.03.2023.
//
#include "server.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

// 1 pshared - share semaphore among processes; 1 - initial value
int initialize_semaphore(Game* game) {
    if (sem_init(&game->sem, 1, 1) == -1) {
        perror("sem_init");
        return -1;
    }

    if (sem_init(&game->sem_draw, 1, 1) == -1) {
        perror("sem_init");
        sem_destroy(&game->sem);
        return -1;
    }
    return 0;
}


void destroy_semaphore(Game* game) {
    if (sem_destroy(&game->sem) == -1) {
        perror("sem_destroy");
    }

    if (sem_destroy(&game->sem_draw) == -1) {
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
    *x =  (rand() % (MAP_WIDTH - 5)) + 2;
    *y = (rand() % (MAP_HEIGHT - 5)) + 2;
}

Point generate_random_position(Game *game) {
    Point pos;
    do {
        pos.x = (rand() % (MAP_WIDTH - 5)) + 2;
        pos.y = (rand() % (MAP_HEIGHT - 5)) + 2;
    } while (game->map[pos.y][pos.x] != ' ');
    return pos;
}


void spawn_treasure(Game *game, Treasure *treasure) {
    Point pos = generate_random_position(game);
    treasure->x = pos.x;
    treasure->y = pos.y;
    game->map[pos.y][pos.x] = treasure->type;
}

void check_and_respawn_treasure(Game *game, Player *player) {
    char cell = game->map[player->y][player->x];
    for (int i = 0; i < 10; i++) {
        if (cell == game->treasure[i].type && player->x == game->treasure[i].x && player->y == game->treasure[i].y) {
            player->carried_coins += game->treasure[i].amount;
            spawn_treasure(game, &game->treasure[i]);
            break;
        }
    }
}

void respawn_treasure(Game * game){
    for(int i = 0; i < 10; i++){
        spawn_treasure(game, &game->treasure[i]);
    }
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
        game->players[i].is_alive = false;
        game->players[i].carried_coins = 0;
        game->players[i].brought_coins = 0;
        game->players[i].deaths = 0;
        game->players[i].client_pid = 0;
        memset(game->players[i].fov_map, ' ', 5 * 5);
    }
}


void initialize_treasures(Game * game){
    for (int i = 0; i < 2; i++) {
        game->treasure[i] = (Treasure){'c', COIN, 0, 0};
        spawn_treasure(game, &game->treasure[i]);
        game->treasure[i + 2] = (Treasure){'t', TREASURE, 0, 0};
        spawn_treasure(game, &game->treasure[i + 2]);
        game->treasure[i + 4] = (Treasure){'T', CHEST, 0, 0};
        spawn_treasure(game, &game->treasure[i + 4]);
        game->treasure[i + 6] = (Treasure){'c', COIN, 0, 0};
        spawn_treasure(game, &game->treasure[i]);
        game->treasure[i + 8] = (Treasure){'t', TREASURE, 0, 0};
        spawn_treasure(game, &game->treasure[i + 2]);
    }
}



void initialize_game_state(Game* game, const char * filename) {

    if(game != NULL && filename != NULL){
        game->game_status = READY; // Example status: game is ready
        game->server_status = READY;
        game->respawn_coins = false;
        game->respawn_beast = false;
        game->server_pid = getpid();
        game->number_of_players = 0;
        memset(game->map, '\0', MAP_HEIGHT * MAP_WIDTH);
        load_map(filename, game);

        for(int i = 0; i < MAP_HEIGHT; i++) {
            game->map[i][MAP_WIDTH - 1] = '\0';
        }

        // AFTER LOADING MAP
        pthread_mutex_init(&game->mutex, NULL);
        pthread_cond_init(&game->beast_cond, NULL);
        pthread_cond_init(&game->game_end_cond, NULL);
        init_players(game);
        initialize_beasts(game);
        initialize_treasures(game);

        ///campsite
        Point campsite = generate_random_position(game);
        game->campsite_y = campsite.y;
        game->campsite_x = campsite.x;

        // Initialize coin drops
        for(int i = 0; i < 5; i++){
            game->drops[i].x = 0;
            game->drops[i].y = 0;
            game->drops[i].value = 0;
            game->drops[i].used = false;
        }



        return;
    }
    printf("error with initialization!");
}

// Function to check collision between two players
bool player_collision(Player *player1, Player *player2) {
    return (player1->x == player2->x) && (player1->y == player2->y);
}

CoinDrop * create_coin_drop(int x, int y, int total , Game * game){
    for(int i = 0; i < 5; i++){
        if(game->drops[i].used == false){
            game->drops[i].value = total;
            game->drops[i].x = x;
            game->drops[i].y = y;
            game->drops->used = true;
            return &game->drops[i];
        }
    }
    return NULL;
}

// Function to handle collision and drop coins
void handle_collision(Game *game, Player *player1, Player *player2) {
    if (player_collision(player1, player2)) {
        int total_coins = player1->carried_coins + player2->carried_coins;

        if(total_coins > 0){
            // Assuming you have a CoinDrop struct with x, y and value properties
            CoinDrop *drop = create_coin_drop(player1->x, player1->y, total_coins, game);
            if(drop == NULL){
                return;
            }

            // Reset player coins
            player1->carried_coins = 0;
            player2->carried_coins = 0;

            //respawn them to original points
            player1->x = player1->spawn_x;
            player1->y = player1->spawn_y;
            player2->x = player2->spawn_x;
            player2->y = player2->spawn_y;
        }
    }
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
                    //game->map[game->players[i].y][game->players[i].x] = ' ';

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



void respawn_beast(Game * game){
    Point pos = generate_random_position(game);
    game->beast.current_pos.x = pos.x;
    game->beast.current_pos.y = pos.y;
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

        //printf("[BEAST][%d][%d]\n", game-> beast.current_pos.x, game->beast.current_pos.y);
        // clean the spot
        //game->map[game->beast.current_pos.y][game->beast.current_pos.x] = ' ';

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
            create_coin_drop(game->players[player_id].x,game->players[player_id].y, game->players[player_id].carried_coins, game);
            game->players[player_id].carried_coins = 0;
            game->players[player_id].x =  game->players[player_id].spawn_x;
            game->players[player_id].y =  game->players[player_id].spawn_y;
            game->players[player_id].deaths++;
            /// TODO: respawn beast somewhere else
            respawn_beast(game);

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

void generate_player_fov_map(Game *game, Player *player) {
    int player_x = player->x;
    int player_y = player->y;

    for (int y = 0; y < 5; y++) {
        for (int x = 0; x < 5; x++) {
            int map_x = player_x - 2 + x;
            int map_y = player_y - 2 + y;

            if (map_x >= 0 && map_x < MAP_WIDTH && map_y >= 0 && map_y < MAP_HEIGHT) {
                player->fov_map[y][x] = game->map[map_y][map_x];
            } else {
                player->fov_map[y][x] = ' '; // Fill out-of-bounds areas with empty space
            }
        }
    }
}

void listen_to_client_connections(Game* game) {

    while(true){

        if (sem_wait(&game->sem) == -1) {
            perror("sem_wait");
            break;
        }

        if (game->server_status == READY){
            fflush(stdout);

            for(int j = 0; j < MAX_PLAYERS; j++ ){
                if(game->players[j].isAssigned == true){
                    ///send kill signal with flag to not kill to check if a client is running
                    if(kill(game->players[j].client_pid, 0) != 0){
                        //free the spot
                        game->players[j].isAssigned = false;
                        game->players[j].carried_coins = 0;
                        game->players[j].client_pid = 0;
                        game->players[j].brought_coins = 0;
                        game->players[j].deaths = 0;
                        game->players[j].is_alive = false;
                        --game->number_of_players;
                    }
                }

            }

            // client sent a request for creating a player. The server has to constantly look for this request.
            for(int i = game->number_of_players; i < MAX_PLAYERS; i++){
                if(game->players[i].isAssigned == true){
                    game->number_of_players++; // increment player count
                    //initialize new player
                    game->players[i].id = i;
                    game->players[i].is_alive = true;
                }
            }
        }
        if(game->server_status == GAME_END){
            // Release the semaphore
            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }
            break;
        }
        // Release the semaphore
        if (sem_post(&game->sem) == -1) {
            perror("sem_post");
            break;
        }
    }

    mvprintw(MAP_HEIGHT + 11, 0, "CLIENT LISTENER THREAD CLOSED");
}

int find_free_spot(Game * game){
    if(game == NULL){
        return -1;
    }
    for(int i = 0; i < MAX_PLAYERS; i++){
        if(!game->players[i].isAssigned){
            return i;
        }
    }
    return -1;
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

        //clear before updating
        clear_positions_on_map(game);

        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == TRUE){
                game->map[game->players[i].y][game->players[i].x] = (char)('0' + game->players[i].id);
            }
        }
        // Update the single beast's position on the map
        if(game->beast.isAssigned == TRUE){
            game->map[game->beast.current_pos.y][game->beast.current_pos.x] = '*';
        }

        // Update treasure positions on the map
        for (int i = 0; i < 10; i++) {
            game->map[game->treasure[i].y][game->treasure[i].x] = game->treasure[i].type;
        }

        game->map[game->campsite_y][game->campsite_x] = 'A';

        for(int i = 0; i < 5; i++){
            if(game->drops[i].used){
                game->map[game->drops[i].y][game->drops[i].x] = 'D';
            }
        }

        return 0;
    }
    return -1;
}

void update_fov(Game * game){
    if(game!= NULL){
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == TRUE){
                generate_player_fov_map(game, &game->players[i]);
            }
        }
    }
}



Game* connect_to_shared_memory(int* shmid) {
    *shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
    if (*shmid < 0) {
        return NULL;
    }

    Game* game = (Game*)shmat(*shmid, NULL, 0);
    if (game == (Game*)-1) {
        return NULL;
    }

    return game;
}

///TODO: dodaj wątek do kazdego nowego dolaczajcego
/// klienta aby serwer mogl updatowac mape w jednej turze
/// dla wszystkich klientow

void clear_positions_on_map(Game * game) {
    if (game != NULL) {
        for (int y = 0; y < MAP_HEIGHT; y++) {
            for (int x = 0; x < MAP_WIDTH; x++) {
                // If the current cell is not a wall, set it to a space character
                if (game->map[y][x] != 'W') {
                    game->map[y][x] = ' ';
                }
            }
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

        if(game->server_status == GAME_END){
            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                pthread_mutex_unlock(&game->mutex);
                break;
            }
            break;
        }

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
    mvprintw(MAP_HEIGHT + 10, 0, "BEAST BEHAVIOR THREAD CLOSED");
    return NULL;
}

void draw_map(Game * game) {

    if(game == NULL)
        return;

    for (int y = 1; y < MAP_HEIGHT - 1; y++) {
        for (int x = 0; x < MAP_WIDTH - 1; x++) {
            char tile = game->map[y][x];
            if (tile == 'W') {
                mvaddch(y, x, '#' | A_BOLD | COLOR_PAIR(1));
            } else {
                mvaddch(y, x, tile);
            }
        }
    }
    // Display player information below the map
    mvprintw(MAP_HEIGHT + 1, 0, "Server status: %d", game->server_status);
    mvprintw(MAP_HEIGHT + 2, 0, "Respawn beast: %d", game->respawn_beast);
    mvprintw(MAP_HEIGHT + 3, 0, "Respawn coins: %d", game->respawn_coins);
    for(int i = 0; i < MAX_PLAYERS; i++){
        mvprintw(MAP_HEIGHT + 4 + i, 0, "Client_%d PID: %d ", i, game->players[i].client_pid);
    }


}

void *redraw_map_thread(void *data) {
    Game *game = (Game *)data;
    while (true) {
        if (sem_wait(&game->sem_draw) == -1) {
            perror("sem_wait");
            break;
        }
        //update_map(game);
        //update_fov(game);
        draw_map(game);

        if(game->server_status == GAME_END){
            if (sem_post(&game->sem_draw) == -1) {
                perror("sem_post");
                break;
            }
            refresh();
            break;
        }
        if (sem_post(&game->sem_draw) == -1) {
            perror("sem_post");
            break;
        }
        //print_map_debug_client();
        //erase();
        refresh();// Redraw every 100ms, adjust this value as needed
    }
    mvprintw(MAP_HEIGHT + 8, 0, "REDRAW THREAD CLOSED");
    return NULL;
}


void *keyboard_listener(void *arg) {
    Game *game = (Game *)arg;

    while (1) {
        int ch = getchar();
        if (ch == 'q') {
            if (sem_wait(&game->sem) == -1) {
                perror("sem_wait");
                break;
            }

            game->server_status = GAME_END;

            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }

            break;
        }
        else if(ch == 'c' || ch == 't' || ch == 'T'){
            if (sem_wait(&game->sem) == -1) {
                perror("sem_wait");
                break;
            }

            game->respawn_coins = true;

            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }

        }
        else if(ch == 'b' || ch == 'B'){
            if (sem_wait(&game->sem) == -1) {
                perror("sem_wait");
                break;
            }

            game->respawn_beast = true;

            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }
        }
    }
    mvprintw(MAP_HEIGHT + 9, 0, "KEYBOARD LISTENER CLOSED");
    return NULL;
}


int run_server() {
    Game* game;

    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);


    int shmid = create_and_attach_shared_memory(&game);
    if (shmid < 0) {
        clear();
        mvprintw(1, 0, "Error creating shared memory");
        refresh();
        endwin(); // Move this line before printing the message
        return 0;
    }

    int ret = initialize_semaphore(game);
    if(ret == -1){
        clear();
        mvprintw(1, 0, "Error initializing semaphores");
        refresh();
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }
    /// --------------------------------

    if (sem_wait(&game->sem) == -1) {
        clear();
        mvprintw(1, 0, "Error with sem_wait in run_server:849");
        refresh();
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }

    initialize_game_state(game, "/mnt/c/Users/wlodi/CLionProjects/mamdosc/mapEmpty.txt");

    if (sem_post(&game->sem) == -1) {
        clear();
        mvprintw(1, 0, "Error with sem_post in run_server:862");
        refresh();
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }
    /// ---------------------------------
    pthread_t client_requests_thread;
    int err = pthread_create(&client_requests_thread, NULL, (void *) listen_to_client_connections, game);
    if (err != 0) {
        clear();
        mvprintw(1, 0, "pthread_create: client_requests_thread run_server:874");
        refresh();
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }

    pthread_t beast_thread;
    err = pthread_create(&beast_thread, NULL, beast_behavior_thread, game);
    if (err != 0) {
        clear();
        mvprintw(1, 0, "beast thread");
        refresh();
        //cancel client listener
        pthread_cancel(client_requests_thread);
        pthread_join(client_requests_thread, NULL);
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }

    pthread_t redraw_thread;
    err = pthread_create(&redraw_thread, NULL, redraw_map_thread, game);
    if (err != 0) {
        clear();
        mvprintw(1, 0, "pthread_create: redraw_thread run_server:901");
        refresh();
        //cancel beast thread
        pthread_cancel(beast_thread);
        pthread_join(beast_thread, NULL);
        //cancel client request thread
        pthread_cancel(client_requests_thread);
        pthread_join(client_requests_thread, NULL);
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }

    pthread_t keyboard_listener_thread;
    if (pthread_create(&keyboard_listener_thread, NULL, keyboard_listener, (void *)game) != 0) {
        clear();
        mvprintw(1, 0, "pthread_create: client_requests_thread run_server:874");
        refresh();
        //cancel beast thread
        pthread_cancel(beast_thread);
        pthread_join(beast_thread, NULL);
        pthread_cancel(client_requests_thread);
        pthread_join(client_requests_thread, NULL);
        pthread_cancel(redraw_thread);
        pthread_join(redraw_thread, NULL);
        destroy_semaphore(game);
        detach_and_remove_shared_memory(shmid, game);
        endwin(); // Move this line before printing the message
        return 0;
    }

    ///TODO: zabezpieczenia i uwalnianie rzeczy
    while (1) {
        if (sem_wait(&game->sem) == -1) {
            perror("sem_wait");
            break;
        }


        process_player_move_request(game);

        //check treasures
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->players[i].isAssigned) {
                check_and_respawn_treasure(game, &game->players[i]);
                ///check if players are at campsite
                if(game->players[i].x == game->campsite_x && game->players[i].y == game->campsite_y){
                    game->players[i].brought_coins += game->players[i].carried_coins;
                    game->players[i].carried_coins = 0;
                }
            }
        }

        //check for player collisions
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->players[i].isAssigned) {
                for(int j = 0; j < MAX_PLAYERS; j++)
                    if (game->players[j].isAssigned && j != i) {
                        if(player_collision(&game->players[i], &game->players[j])){
                            handle_collision(game, &game->players[i], &game->players[j]);
                    }
                }
            }
        }

        //check if players picked up coin drops
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (game->players[i].isAssigned) {
                for(int j = 0; j < 5; j++){
                    if(game->drops[j].used){
                        if(game->players[i].x == game->drops[j].x && game->players[i].y == game->drops[j].y){
                            game->players[i].carried_coins += game->drops[j].value;
                            game->drops[j].value = 0;
                            game->drops[j].used = false;
                        }
                    }
                }
            }
        }

        if (pthread_mutex_lock(&game->mutex) != 0) {
            perror("pthread_mutex_lock");
            break;
        }

        if (game->server_status == GAME_END) {
            pthread_cond_signal(&game->beast_cond);
            if (pthread_mutex_unlock(&game->mutex) != 0) {
                perror("pthread_mutex_unlock");
                break;
            }
            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                break;
            }
            break;
        }
        else if(game->respawn_beast){
            game->respawn_beast = false;
            respawn_beast(game);
        }
        else if(game->respawn_coins){
            game->respawn_coins = false;
            respawn_treasure(game);
        }

        if (pthread_mutex_unlock(&game->mutex) != 0) {
            perror("pthread_mutex_unlock");
            break;
        }

        update_map(game);
        update_fov(game);
        //print_map_debug(game);

        if (sem_post(&game->sem) == -1) {
            perror("sem_post");
            break;
        }

        // Signal the client to draw the map
        if (sem_post(&game->sem_draw) == -1) {
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
        usleep(500000);
    }

    // Before exiting the server
    mvprintw(MAP_HEIGHT + 7, 0, "GAME ENDED QUITING...");
    refresh();

    pthread_join(keyboard_listener_thread, NULL);
    pthread_join(beast_thread, NULL);
    pthread_join(redraw_thread, NULL);
    pthread_join(client_requests_thread, NULL);


    // Destroy the mutex before the program exits
    destroy_semaphore(game);
    pthread_mutex_destroy(&game->mutex);
    pthread_cond_destroy(&game->game_end_cond);
    pthread_cond_destroy(&game->beast_cond);
    //destroy shared memory
    detach_and_remove_shared_memory(shmid, game);


    endwin(); // Move this line after printing the message and refreshing the screen

    return 0;
}