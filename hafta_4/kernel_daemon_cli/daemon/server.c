#include "declarations.h"
#include "headers.h"
#include "become_daemon.h"

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

/* Tek satırlık komutu işler: set-tcp <p>, set-udp <p>, clear, stats */
static int pktfilter_handle_cmd(const char* line, char* out, size_t cap)
{
    if (!line || !out || cap == 0) return -1;

    /* kopya al (strtok inputu değiştirir) */
    char tmp[256];
    size_t len = strnlen(line, sizeof(tmp)-1);
    memcpy(tmp, line, len);
    tmp[len] = '\0';

    /* satır sonu boşluklarını kırp */
    while (len && (tmp[len-1]=='\r' || tmp[len-1]=='\n' || tmp[len-1]==' ' || tmp[len-1]=='\t'))
        tmp[--len] = '\0';

    char *save = NULL;
    char *cmd = strtok_r(tmp, " \t", &save);
    if (!cmd) return -1;

    /* Bu komutlardan biri değilse -1 dön (server kendi komutlarına baksın) */
    int is_pf =
        !strcmp(cmd, "set-tcp") ||
        !strcmp(cmd, "set-udp") ||
        !strcmp(cmd, "clear")   ||
        !strcmp(cmd, "stats");
    if (!is_pf) return -1;

    int fd = open(DEV_PATH, O_RDWR);
    if (fd < 0) {
        return snprintf(out, cap, "ERR open(%s): %s\n", DEV_PATH, strerror(errno));
    }

    int rc = 0;
    if (!strcmp(cmd, "set-tcp")) {
        char *arg = strtok_r(NULL, " \t", &save);
        if (!arg) { close(fd); return snprintf(out, cap, "ERR usage: set-tcp <port>\n"); }
        int port = atoi(arg);
        rc = ioctl(fd, PKTFILT_IOC_SET_TCP_BLOCK, &port);
        if (rc < 0) snprintf(out, cap, "ERR ioctl SET_TCP: %s\n", strerror(errno));
        else        snprintf(out, cap, "OK tcp_block=%d\n", port);

    } else if (!strcmp(cmd, "set-udp")) {
        char *arg = strtok_r(NULL, " \t", &save);
        if (!arg) { close(fd); return snprintf(out, cap, "ERR usage: set-udp <port>\n"); }
        int port = atoi(arg);
        rc = ioctl(fd, PKTFILT_IOC_SET_UDP_BLOCK, &port);
        if (rc < 0) snprintf(out, cap, "ERR ioctl SET_UDP: %s\n", strerror(errno));
        else        snprintf(out, cap, "OK udp_block=%d\n", port);

    } else if (!strcmp(cmd, "clear")) {
        rc = ioctl(fd, PKTFILT_IOC_CLEAR_STATS);
        if (rc < 0) snprintf(out, cap, "ERR ioctl CLEAR: %s\n", strerror(errno));
        else        snprintf(out, cap, "OK cleared\n");

    } else if (!strcmp(cmd, "stats")) {
        struct pkt_stats s = {0};
        rc = ioctl(fd, PKTFILT_IOC_GET_STATS, &s);
        if (rc < 0) snprintf(out, cap, "ERR ioctl GET_STATS: %s\n", strerror(errno));
        else        snprintf(out, cap,
                    "total=%llu tcp_pass=%llu udp_pass=%llu tcp_drop=%llu udp_drop=%llu\n",
                    (unsigned long long)s.total,
                    (unsigned long long)s.tcp_pass,
                    (unsigned long long)s.udp_pass,
                    (unsigned long long)s.tcp_drop,
                    (unsigned long long)s.udp_drop);
    }

    close(fd);
    return 0; /* out’a mesaj yazıldı */
}
/* ==== /pktfilter ==== */
int monitored_fd_set[MAX_CLIENT_SUPPORTED];
int client_result[MAX_CLIENT_SUPPORTED] = {0};

static void initialize_monitor_fd_set() {
  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    monitored_fd_set[i] = -1;
  }
}

static void add_to_monitored_fd(int skt_fd) {
  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] != -1)
      continue;
    monitored_fd_set[i] = skt_fd;
    break;
  }
}

static void remove_from_monitored_fd(int skt_fd) {
  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] != skt_fd)
      continue;
    monitored_fd_set[i] = -1;
    break;
  }
}

static void refresh_fd_set(fd_set *fd_set_ptr) {
  FD_ZERO(fd_set_ptr);
  //    void FD_ZERO(fd_set *set);

  int i = 0;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] != -1)
      FD_SET(monitored_fd_set[i], fd_set_ptr);
  }
  // void FD_SET(int fd, fd_set *set);
}

static int get_max_fd() {
  int i = 0;
  int max = -1;
  for (; i < MAX_CLIENT_SUPPORTED; i++) {
    if (monitored_fd_set[i] > max)
      max = monitored_fd_set[i];
  }
  return max;
}

void handle_and_respond_command(int comm_fd, const char* command) {
    char buffer[BUFFER_SIZE];
    
    /* === ÖNCE pktfilter komutları mı diye kontrol et === */
    {
        char pf_reply[256];
        int pf = pktfilter_handle_cmd(command, pf_reply, sizeof(pf_reply));
        if (pf == 0) {
            /* pktfilter komutuydu, cevabı gönder ve bitir */
            int wr = write(comm_fd, pf_reply, strlen(pf_reply) + 1);
            if (wr < 0) { perror("write"); }
            
        } else {
          log_level(LOG_LEVEL_DEBUG, "Unknown pktfilter command: %s", command);
        }
        
        /* pf == -1 ise bizim komutumuz değil, alttaki mevcut if/else'lere devam */
    }
    /* === /pktfilter === */
    // Komuta göre yanıt belirle
    if (strcmp(command, "show mem") == 0) {
        char *memLine = getMemInfo();
        snprintf(buffer, BUFFER_SIZE, "%s", memLine);
    } else if (strcmp(command, "show cpu") == 0) {
        double cpuPct = getCpuUsage();
        snprintf(buffer, BUFFER_SIZE,"CPU Usage: %.2f%%", cpuPct);
    } else if (strcmp(command, "loginfo") == 0) {
        snprintf(buffer, BUFFER_SIZE,"loglevel is: %d", current_log_level);
    } else if (strcmp(command, "show temp") == 0) {
        double cpuTemp = getCpuTemperature();
        snprintf(buffer, BUFFER_SIZE,"CPU Tempature: %.2f°C", cpuTemp);
    } else if (strncmp(command, "loglevel", 8) == 0) {
    char *arg = strchr(command, ' ');
    if (!arg) {
        snprintf(buffer, BUFFER_SIZE,
                 "Usage: loglevel <error|warning|info|debug>");
      } else {
          arg++;
          if (strcasecmp(arg, "error") == 0)      current_log_level = LOG_LEVEL_ERROR;
          else if (strcasecmp(arg, "warning") == 0) current_log_level = LOG_LEVEL_WARNING;
          else if (strcasecmp(arg, "info") == 0)    current_log_level = LOG_LEVEL_INFO;
          else if (strcasecmp(arg, "debug") == 0)   current_log_level = LOG_LEVEL_DEBUG;
          else {
              snprintf(buffer, BUFFER_SIZE, "Unknown log level: %s", arg);
              write(comm_fd, buffer, strlen(buffer)+1);
              return;
          }
          snprintf(buffer, BUFFER_SIZE, "Log level set to %s", arg);
      }
    } else if (strcmp(command, "help") == 0) {
        snprintf(buffer, BUFFER_SIZE, "Commands: show mem | show cpu | show temp | loginfo | set-tcp <p> | set-udp <p> | stats| clear | loglevel <debug/info/warning/error> | help");
    } else {
        snprintf(buffer, BUFFER_SIZE, "Error: Unknown command (%s)", command);
        
        log_level(LOG_LEVEL_DEBUG, "Unknown command (fd: %d): %s", comm_fd, command);
        log_level(LOG_LEVEL_ERROR, "Unknown command (fd: %d): %s", comm_fd, command);
        log_level(LOG_LEVEL_INFO, "Unknown command (fd: %d): %s", comm_fd, command);

    }

    printf("Yanıt gönderiliyor: %s\n", buffer);

    // Yanıtı client'a gönder
    int ret = write(comm_fd, buffer, strlen(buffer) + 1);
    if (ret < 0) {
        log_level(LOG_LEVEL_ERROR, "Write respond command to client (fd: %d): %s", comm_fd, strerror(errno));
        perror("write");
        exit(EXIT_FAILURE);
    }

    

    // Bağlantıyı kapat
    //close(comm_fd);
    //remove_from_monitored_fd(comm_fd);
}


int main(int argc, char *argv[]) {
  struct sockaddr_un name;
  // Initialization
  int connection_socket, i;
  int data_socket;
  int comm_fd;
  int ret;
  fd_set readfds;
  char buffer[BUFFER_SIZE];
  const int SLEEP_TIME = 5;
  openlog("netfilter-server", LOG_PID | LOG_CONS, LOG_USER);
  //setlogmask(LOG_UPTO(LOG_DEBUG));
  log_level(LOG_LEVEL_INFO, "Server created successfuly.");
  if (becomeDaemon(0) == -1)
  errExit("becomeDaemon");
  log_level(LOG_LEVEL_INFO, "Server working as a daemon.");

  initialize_monitor_fd_set();

  unlink(SOCKET_NAME);
  // create master_fd socket
  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_socket < 0) {
    perror("socket");
    log_level(LOG_LEVEL_ERROR, "Socket failed.");
    exit(EXIT_FAILURE);
  }
  printf("Socket created successfully\n");
  memset(&name, 0, sizeof(struct sockaddr_un));
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, SOCKET_NAME, sizeof(name.sun_path) - 1);

  ret = bind(connection_socket, (struct sockaddr *)&name,
             sizeof(struct sockaddr_un));
  if (ret == -1) {
    perror("bind");
    log_level(LOG_LEVEL_ERROR, "Socket bind error: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }
  printf("Socket binding successful \n");

  ret = listen(connection_socket, 20);
  if (ret == -1) {
    perror("listen");
    log_level(LOG_LEVEL_ERROR, "Socket listen error: %s", strerror(errno));
    exit(EXIT_FAILURE);
  }

  add_to_monitored_fd(connection_socket);




  logOpen("/tmp/error.log");
  //readConfigFile("/tmp/ds.conf");

  logMessage("MEMORY INFO");

  for (;;) {
    refresh_fd_set(&readfds);
    struct timeval tv;
    tv.tv_sec  = SLEEP_TIME;
    tv.tv_usec = 0;
    int ready = select(get_max_fd() + 1, &readfds, NULL, NULL, &tv);
    /***
           int select(int nfds, fd_set *_Nullable restrict readfds,
                      fd_set *_Nullable restrict writefds,
                      fd_set *_Nullable restrict exceptfds,
                      struct timeval *_Nullable restrict timeout);
    ****/
    // select is a blocking call, server waits for request from the client
    //        int  FD_ISSET(int fd, fd_set *set);
    if (ready == -1) {
    if (errno == EINTR)
        continue;   // sinyal kesmesi olduysa yeniden dene
    perror("select");
    break;
    }

    if (ready == 0) {
        // **Periyodik**: CPU % ve bellek bilgilerini birlikte logla
        double cpuPct = getCpuUsage();
        double cpuTemp = getCpuTemperature();
        char *memLine = getMemInfo();
        if (memLine && cpuPct && cpuTemp) {
            logMessage("Cpu Temp: %.2f\nCPU Usage: %.2f%%\nMemInfo:\n%s",
                        cpuTemp, cpuPct, memLine);
        } else {
            logMessage("info not found)");
        }
        continue;
    }
    if (FD_ISSET(connection_socket, &readfds)) {
      // if the fd is same as master fd of the socket ==> implies this is a
      // request from new client
      printf("Connection request from a new client , accept the request\n");
      log_level(LOG_LEVEL_INFO, "Connection request from a new client , accept the request.");

      // aacept the request

      data_socket = accept(connection_socket, NULL, NULL);
      if (data_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }
      printf("Conenction established for new client\n");
      log_level(LOG_LEVEL_INFO, "Conenction established for new client.");
      add_to_monitored_fd(data_socket);

    } else // fd already exist in the fd _set ==> it's an request from a client
           // laready connected
    {
      // find the client fd in the fd_set
      i = 0;
      comm_fd = -1;
      for (; i < MAX_CLIENT_SUPPORTED; i++) {
        if (FD_ISSET(monitored_fd_set[i], &readfds)) {
          comm_fd = monitored_fd_set[i];

          // read the data from the comm_fd
          memset(buffer, 0, BUFFER_SIZE);

          // waiting to read the data , read is a block call

          printf("Waitng for the data\n");
          ret = read(comm_fd, buffer, BUFFER_SIZE);
          if (ret == -1) {
            perror("read");
            exit(EXIT_FAILURE);
          }
          // copy the read data from buffer ==> data , design is expecting only
          // int data
       

          handle_and_respond_command(comm_fd, buffer);
          continue;
 
        }
      }
    }
  } // go to select bloc syscall

  close(connection_socket);
  remove_from_monitored_fd(connection_socket);
  log_level(LOG_LEVEL_INFO, "Connection closed.");
  printf("Connection closed\n");
  unlink(SOCKET_NAME);
  closelog();
  exit(EXIT_SUCCESS);
}