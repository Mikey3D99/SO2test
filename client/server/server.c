//
// Created by wlodi on 26.03.2023.
//
#include "server.h"

sem_t sem;

// 1 pshared - share semaphore among processes; 1 - initial value
int initialize_semaphore() {
    if (sem_init(&sem, 1, 1) == -1) {
        perror("sem_init");
        return -1;
    }
    return 0;
}

void destroy_semaphore() {
    if (sem_destroy(&sem) == -1) {
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

void initialize_game_state(Game* game) {
    game->game_status = READY; // Example status: game is ready
    game->server_status = READY;
    game->next_move = NONE;
    game->current_player = NULL;
    memset(game->map, ' ', MAP_HEIGHT * MAP_WIDTH);
    for(int i = 0; i < MAX_PLAYERS; i++){
        game->players[i].isAssigned = false;
    }
}


char get_next_move_char(Game * game){
    char for_return = ' ';
    int x = game->current_player->x;
    int y = game->current_player->y;

    switch(game->next_move){
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
    return for_return;
}

bool is_move_allowed(Game * game){
    if(game != NULL){
        if(game->current_player != NULL){
            if(game->next_move == NONE){
                return true;
            }
            else{
                char obstacles[] = "W#";
                char next_move = get_next_move_char(game);

                // check if the next move is one of the obstacles
                if (strchr(obstacles, next_move) != NULL) {
                    return false;
                }
                else {
                    return true;
                }
            }
        }
    }
    return false;
}


int process_player_move_request(Game * game){
    if(game != NULL){
        if(is_move_allowed(game)){

            int x = game->current_player->x;
            int y = game->current_player->y;
            int id = game->current_player->id;

            switch(game->next_move){
                case UP:
                    game->map[y - 1][x] = (char)('0' + id);
                    break;
                case DOWN:
                    game->map[y + 1][x] = (char)('0' + id);
                    break;
                case RIGHT:
                    game->map[y][x + 1] = (char)('0' + id);
                    break;
                case LEFT:
                    game->map[y][x - 1] = (char)('0' + id);
                    break;
                case NONE:
                    break;
            }

            // change the previous char to empty space
            game->map[y][x] = ' ';
            return 0;
        }
    }
    return -1;
}

void process_client_requests(Game* game) {
    while (1) {

        if (game->server_status == READY){
            printf("server is running...\n");
            fflush(stdout);

            /*
            /// ACCESS TO SHARED MEM
            if (sem_wait(&sem) == -1) {
                perror("sem_wait");
                return;
            }*/
            process_player_move_request(game);
            update_map(game);
            /*
            /// release
            if (sem_post(&sem) == -1) {
                perror("sem_post");
                return;
            }*/

            if (game->game_status == GAME_END) { // Example status: end the game
                break;
            }
        }
        usleep(1000); // Sleep for a short duration to reduce CPU usage
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


int update_map(Game * game){
    if(game != NULL){
        for(int i = 0; i < MAX_PLAYERS; i++){
            if(game->players[i].isAssigned == TRUE){
                game->map[game->players[i].y + 10][game->players[i].x + 10] = (char)('0' + game->players[i].id);
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


void run_server() {
    Game* game;
    int shmid = create_and_attach_shared_memory(&game);
    if (shmid < 0) {
        exit(1);
    }

    initialize_game_state(game);
    initialize_semaphore();
    process_client_requests(game);

    // Set the server status to 0 to signal the update_server_status thread to exit
    game->server_status = 0;
    detach_and_remove_shared_memory(shmid, game);
    destroy_semaphore();
}