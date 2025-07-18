// test_become_daemon.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "become_daemon.h"

int main(int argc, char *argv[]) {
    // become daemon
    if (becomeDaemon(0) == -1) {
        perror("becomeDaemon");
        exit(EXIT_FAILURE);
    }
    //If there is an argument, take it as seconds, otherwise sleep for 20 seconds.
    int sleepTime = 20;
    if (argc > 1) {
        sleepTime = atoi(argv[1]);
        if (sleepTime <= 0) {
            fprintf(stderr, "Geçersiz uyuma süresi: %s\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    }

    sleep(sleepTime);

    exit(EXIT_SUCCESS);
}
