//
// Created by wlodi on 26.03.2023.
//
#include "server.h"

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
    for(int i = 0; i < MAX_PLAYERS; i++){
        game->players[i].isAssigned = false;
    }
}

void process_client_requests(Game* game) {
    while (1) {

        if (game->server_status == READY){
            printf("server is running...\n");
            fflush(stdout);
            if (game->game_status == MOVED) {
                // Example status: client made a move
                // Process the client's move and update the game state
                // ...

                // Set the game status back to ready
                game->game_status = READY;
            }

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
                map[game->players[i].y + 10][game->players[i].x + 10] = (char)('0' + game->players[i].id);
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
    process_client_requests(game);

    // Set the server status to 0 to signal the update_server_status thread to exit
    game->server_status = 0;
    detach_and_remove_shared_memory(shmid, game);
}