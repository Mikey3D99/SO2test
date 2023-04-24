//
// Created by wlodi on 26.03.2023.
//
#include "server.h"
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

sem_t sem;
int current_number_of_players = 0;

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
    *x = (rand() %(MAP_WIDTH - 3 + 1)) + 3;
    *y = (rand() %(MAP_HEIGHT - 3 + 1)) + 3;
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

    for (int i = 0; i < MAX_BEASTS; i++) {
        game->beasts[i].id = i;
        int x, y;

        do {
            random_coordinates(&x, &y);
        } while (!is_valid_position(game, x, y));

        game->beasts[i].current_pos.x = x;
        game->beasts[i].current_pos.y = y;
        game->beasts[i].isAssigned = false;
        game->beasts[i].next_move = NONE;

        if(i == 0)
            game->beasts[i].isAssigned = true;
    }
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
        game->players[i].isAssigned = false;
        game->players[i].next_move = NONE;
    }
}



void initialize_game_state(Game* game, const char * filename) {

    if(game != NULL && filename != NULL){
        game->game_status = READY; // Example status: game is ready
        game->server_status = READY;
        game->create_player_id = -1;
        game->create_player = false;
        game->created = false;
        memset(game->map, ' ', MAP_HEIGHT * MAP_WIDTH);
        load_map(filename, game);

        // AFTER LOADING MAP
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
        printf("[%d]", player->next_move);
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

Player * find_free_spot(Game * game){
    if(game == NULL){
        return NULL;
    }

    for(int i = 0; i < MAX_PLAYERS; i++){
        Player * current = &game->players[i];
        if(!current->isAssigned){
            return current;
        }
    }

    return NULL;
}

Player * add_player(int id, Game * game){
    if(game == NULL){
        return NULL;
    }

    if(id >= 0 && id < 4){
        game->players[id].isAssigned = true;
    }
    return NULL;
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

int find_closest_player_id(Game * game, int beast_id){
    if(game == NULL){
        return -1;
    }

    Beast * current_beast = &game->beasts[beast_id];
    int point_number = 0;
    Player * min_player = NULL;
    int min_distance = 0;

    for(int i = 0; i < MAX_PLAYERS; i++){
        if(game->players[i].isAssigned){
            Point * points = bresenham(game->players[i].x, game->players[i].y,
                                         current_beast->current_pos.x, current_beast->current_pos.y,
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
    return min_player->id;
}


// byc moze tutaj osobny watek aby bestia patrzyla caly czas czy jakis gracz nie wszedl w jej sight
void beast_follow_player(int beast_id, int player_id, Game * game){
    if(game == NULL){
        return;
    }

    int distanceX = abs(game->players[player_id].x - game->beasts[beast_id].current_pos.x);
    int distanceY = abs(game->players[player_id].y - game->beasts[beast_id].current_pos.y);
    // nie ma scian i jest w zasiegu
    if(!is_player_obstructed_by_wall(game->players[player_id].x,
                                    game->players[player_id].y,
                                    game->beasts[beast_id].current_pos.x,
                                    game->beasts[beast_id].current_pos.y, game, 7)){

        // clean the spot
        game->map[game->beasts[beast_id].current_pos.y][game->beasts[beast_id].current_pos.x] = ' ';

        int beast_x = game->beasts[beast_id].current_pos.x;
        int beast_y = game->beasts[beast_id].current_pos.y;
        int player_x = game->players[player_id].x;
        int player_y = game->players[player_id].y;

        //zanim zmieni pozycje bestii sprawdz czy nie ma sciany
        if (distanceX > distanceY) {
            beast_x += (game->players[player_id].x > game->beasts[beast_id].current_pos.x) ? 1 : -1;
        } else {
            beast_y += (game->players[player_id].y > game->beasts[beast_id].current_pos.y) ? 1 : -1;
        }
        if(game->map[beast_y][beast_x] != 'W'){
            game->beasts[beast_id].current_pos.x = beast_x;
            game->beasts[beast_id].current_pos.y = beast_y;
        }
    }
}

void process_beasts(Game * game){
    if(game == NULL){
        return;
    }

}

void process_client_requests(Game* game) {
    int ch;
    while (1) { // Exit the loop when the 'q' key is pressed
        if (game->server_status == READY){
            fflush(stdout);

            // Wait for the semaphore
            if (sem_wait(&game->sem) == -1) {
                perror("sem_wait");
                return;
            }

            //ADDING PLAYERS /// TODO: do naprawy, przeciez player juz istnieje w tablicy tylko musi miec zmienione property pod siebie
            Player * newly_created_player = NULL;
            if(game->create_player == true && current_number_of_players < 3){
                newly_created_player = find_free_spot(game);
                add_player(newly_created_player->id, game);
                game->create_player = false;
                game->created = true;
                game->create_player_id = newly_created_player->id;
                current_number_of_players++;
            }


            //TEST BREHNAM
            if(game->players[0].isAssigned && game->beasts[0].isAssigned){
                beast_follow_player(0, 0, game);
            }

            process_player_move_request(game);
            update_map(game);


            // Release the semaphore
            if (sem_post(&game->sem) == -1) {
                perror("sem_post");
                return;
            }
        }
        sleep(1); // Sleep for a short duration to reduce CPU usage
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
        for(int i = 0; i < MAX_BEASTS; i++){
            if(game->beasts[i].isAssigned == TRUE){
                game->map[game->beasts[i].current_pos.y][game->beasts[i].current_pos.x] = '*';
            }
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

void run_server() {
    Game* game;
    int shmid = create_and_attach_shared_memory(&game);
    if (shmid < 0) {
        exit(1);
    }


    initialize_game_state(game, "/mnt/c/Users/wlodi/CLionProjects/SO2/mapEmpty.txt");
    initialize_semaphore(game);
    process_client_requests(game);

    // Set the server status to 0 to signal the update_server_status thread to exit
    game->server_status = 0;
    detach_and_remove_shared_memory(shmid, game);
    destroy_semaphore(game);
}