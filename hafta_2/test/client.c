#include "declarations.h"
#include "headers.h"

int main(int argc, char *argv[]) {
    struct sockaddr_un name;
    int data_socket;
    int ret;
    char buffer[BUFFER_SIZE];

    data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (data_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    printf("Socket created\n");

    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

    ret = connect(data_socket, (const struct sockaddr *)&name, sizeof(struct sockaddr_un));
    if (ret < 0) {
        fprintf(stderr, "The server is down\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server. Type messages (type 'exit' to quit):\n");

    while (1) {
        printf("> ");
        memset(buffer, 0, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE, stdin);

        // Remove trailing newline
        buffer[strcspn(buffer, "\n")] = '\0';

        // Send to server
        ret = write(data_socket, buffer, strlen(buffer) + 1);  // include '\0'
        if (ret == -1) {
            perror("write");
            break;
        }

        // If input is "exit", stop the loop
        if (strcmp(buffer, "exit") == 0)
            break;

        // Receive response
        memset(buffer, 0, BUFFER_SIZE);
        ret = read(data_socket, buffer, BUFFER_SIZE);
        if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("%s\n", buffer);
    }

    close(data_socket);
    exit(EXIT_SUCCESS);
}
