// user.c
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define DEV_PATH "/dev/pktfilter"

#define PKTFILT_IOC_MAGIC   'p'
#define PKTFILT_IOC_SET_TCP_BLOCK   _IOW(PKTFILT_IOC_MAGIC, 1, int)
#define PKTFILT_IOC_SET_UDP_BLOCK   _IOW(PKTFILT_IOC_MAGIC, 2, int)
#define PKTFILT_IOC_GET_STATS       _IOR(PKTFILT_IOC_MAGIC, 3, struct pkt_stats)
#define PKTFILT_IOC_CLEAR_STATS     _IO(PKTFILT_IOC_MAGIC, 4)

struct pkt_stats {
    uint64_t total;
    uint64_t tcp_pass;
    uint64_t udp_pass;
    uint64_t tcp_drop;
    uint64_t udp_drop;
};

static void print_stats(int fd) {
    struct pkt_stats s = {0};
    if (ioctl(fd, PKTFILT_IOC_GET_STATS, &s) == 0) {
        printf("total=%llu tcp_pass=%llu udp_pass=%llu tcp_drop=%llu udp_drop=%llu\n",
               (unsigned long long)s.total,
               (unsigned long long)s.tcp_pass,
               (unsigned long long)s.udp_pass,
               (unsigned long long)s.tcp_drop,
               (unsigned long long)s.udp_drop);
    } else {
        perror("ioctl(GET_STATS)");
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage:\n"
               "  %s set-tcp <port>\n"
               "  %s set-udp <port>\n"
               "  %s stats\n"
               "  %s clear\n", argv[0], argv[0], argv[0], argv[0]);
        return 0;
    }

    int fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    if (!strcmp(argv[1], "set-tcp") && argc == 3) {
        int port = atoi(argv[2]);
        if (ioctl(fd, PKTFILT_IOC_SET_TCP_BLOCK, &port) < 0) perror("ioctl(SET_TCP)");
    } else if (!strcmp(argv[1], "set-udp") && argc == 3) {
        int port = atoi(argv[2]);
        if (ioctl(fd, PKTFILT_IOC_SET_UDP_BLOCK, &port) < 0) perror("ioctl(SET_UDP)");
    } else if (!strcmp(argv[1], "stats")) {
        print_stats(fd);
    } else if (!strcmp(argv[1], "clear")) {
        if (ioctl(fd, PKTFILT_IOC_CLEAR_STATS) < 0) perror("ioctl(CLEAR)");
    } else {
        fprintf(stderr, "Unknown command\n");
    }

    close(fd);
    return 0;
}
