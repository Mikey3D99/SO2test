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

//this only use in add player
Player* create_player(int id) {
    Player* new_player = (Player*)malloc(sizeof(Player));
    new_player->x = id + 10;
    new_player->y = id + 10;
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

int add_player_to_shared_memory(Player *player, Game *shared_memory) {
    if (shared_memory != NULL && player != NULL) {
        for (int i = 0; i < MAX_PLAYERS; i++) {
            if (shared_memory->players[i].isAssigned == FALSE) {
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

void initialize_game_state(Game* game, const char * filename) {

    if(game != NULL && filename != NULL){
        game->game_status = READY; // Example status: game is ready
        game->server_status = READY;
        game->next_move = NONE;
        game->current_player = NULL;
        game->current_player_id = 0;
        game->create_player = false;
        game->created = false;
       // memset(game->map, ' ', MAP_HEIGHT * MAP_WIDTH);

        load_map(filename, game);
        for(int i = 0; i < MAX_PLAYERS; i++){
            game->players[i].isAssigned = false;
        }
        return;
    }
    printf("error with initialization!");
}


char get_next_move_char(Game * game){
    char for_return = 'W';
    int x = game->players[game->current_player_id].x;
    int y = game->players[game->current_player_id].y;
    //printf("%d",  game->current_player->x);
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
                printf("[%c]",  next_move);

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
        if(game->current_player == NULL){
            printf("Current player is null");
            return -1;
        }
        if(is_move_allowed(game)){
            game->map[game->players[game->current_player_id].y][game->players[game->current_player_id].x] = ' ';

            switch(game->next_move){
                case UP:
                    game->players[game->current_player_id].y--;
                    break;
                case DOWN:
                    game->players[game->current_player_id].y++;
                    break;
                case RIGHT:
                    game->players[game->current_player_id].x++;
                    break;
                case LEFT:
                    game->players[game->current_player_id].x--;
                    break;
                case NONE:
                    break;
            }

            // change the previous char to empty space
            return 0;
        }
    }
    return -1;
}

void process_client_requests(Game* game) {
    int ch;
    while ((ch = getch()) != 'q') { // Exit the loop when the 'q' key is pressed
        if (game->server_status == READY){
            printf("server is running...\n");
            fflush(stdout);

            /// ACCESS TO SHARED MEM
            if (sem_wait(&sem) == -1) {
                perror("sem_wait");
                return;
            }

            //ADDING PLAYERS
            if(game->create_player == true){
                game->current_player = add_player(game->current_player, game->current_player_id);
                add_player_to_shared_memory(game->current_player, game);
                game->create_player = false;
                game->created = true;
            }

            if(game->current_player != NULL && game->next_move != NONE){
                process_player_move_request(game);
                game->next_move = NONE;
            }
            update_map(game);
            /// release
            if (sem_post(&sem) == -1) {
                perror("sem_post");
                return;
            }

            if (game->game_status == GAME_END) { // Example status: end the game
                break;
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
    initialize_semaphore();
    process_client_requests(game);

    // Set the server status to 0 to signal the update_server_status thread to exit
    game->server_status = 0;
    detach_and_remove_shared_memory(shmid, game);
    destroy_semaphore();
}