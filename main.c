#include <ncurses.h>
#include "client/server/server.h"
#include "client/player/player.h"

int main(int argc, char *argv[]){

    if (argc != 2) {
        printf("Usage: %s [server|client]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        run_client();
    } else {
        printf("Invalid argument. Usage: %s [server|client]\n", argv[0]);
        return 1;
    }

    return 0;
}