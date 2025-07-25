/* us_xfr_cl.c

   Client for UNIX domain socket data transfer example.
   This client connects to a socket path and sends a message to the server.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "us_xfr.h"

int main(void) {
    int sock;
    struct sockaddr_un addr;
    const char *msg = "Hello from client!";

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    if (write(sock, msg, strlen(msg)) != strlen(msg)) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    printf("Message sent to server.\n");

    close(sock);
    return 0;
}
