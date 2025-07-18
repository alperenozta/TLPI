// become_daemon.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

#define BD_MAX_CLOSE 8192

int becomeDaemon(int flags) {
    int maxfd, fd;

    // 1. first fork
    pid_t pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        exit(EXIT_SUCCESS); // Parent exits

    // 2. create new session
    if (setsid() == -1)
        return -1;

    // 3. second fork for not to be session leader again
 
    pid = fork();
    if (pid < 0)
        return -1;
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // 4. reset unmask
    umask(0);

    // 5. change directory to root
    if (chdir("/") == -1)
        return -1;

    // 6. close open files
    maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1)
        maxfd = BD_MAX_CLOSE;

    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    // 7. direct standart I/O 's to /dev/null
    
    fd = open("/dev/null", O_RDWR);
    if (fd != 0) return -1;
    if (dup2(0, 1) != 1) return -1;
    if (dup2(0, 2) != 2) return -1;

    return 0;
}
