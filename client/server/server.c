//
// Created by wlodi on 26.03.2023.
//
#include "server.h"

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


void init_players(Game* game) {
    if (game == NULL) {
        return;
    }

    for (int i = 0; i < MAX_PLAYERS; i++) {
        game->players[i].id = i;
        game->players[i].x = 10 + i;
        game->players[i].y = 10 + i;
        game->players[i].isAssigned = false;
        game->players[i].next_move = UP;
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
        init_players(game);
        load_map(filename, game);
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
        printf("[%c]",  next_move);

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

void process_client_requests(Game* game) {
    int ch;
    while (1) { // Exit the loop when the 'q' key is pressed
        if (game->server_status == READY){
            printf("server is running...\n");
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