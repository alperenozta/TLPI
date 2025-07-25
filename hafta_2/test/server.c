#include "declarations.h"
#include "headers.h"
#include "become_daemon.h"

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
    
    
    // Komuta göre yanıt belirle
    if (strcmp(command, "show mem") == 0) {
        char *memLine = getMemInfo();
        snprintf(buffer, BUFFER_SIZE, "%s", memLine);
    } else if (strcmp(command, "show cpu") == 0) {
        double cpuPct = getCpuUsage();
        snprintf(buffer, BUFFER_SIZE,"CPU Usage: %.2f%%", cpuPct);
    } else if (strcmp(command, "show temp") == 0) {
        double cpuTemp = getCpuTemperature();
        snprintf(buffer, BUFFER_SIZE,"CPU Tempature: %.2f°C", cpuTemp);
    } else if (strcmp(command, "loginfo") == 0) {
        snprintf(buffer, BUFFER_SIZE, "Log seviyesi INFO olarak ayarlandı.");
    } else if (strcmp(command, "help") == 0) {
        snprintf(buffer, BUFFER_SIZE, "Commands: show mem | show cpu | show temp | loginfo | help");
    } else {
        snprintf(buffer, BUFFER_SIZE, "HATA: Geçersiz komut (%s)", command);
    }

    printf("Yanıt gönderiliyor: %s\n", buffer);

    // Yanıtı client'a gönder
    int ret = write(comm_fd, buffer, strlen(buffer) + 1);
    if (ret < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }

    // Bağlantıyı kapat
    close(comm_fd);
    remove_from_monitored_fd(comm_fd);
}


int main(int argc, char *argv[]) {
  struct sockaddr_un name;
  // Initialization
  int connection_socket, i;
  int data_socket;
  int comm_fd;
  int ret;
  int data;
  int result;
  fd_set readfds;
  char buffer[BUFFER_SIZE];
  const int SLEEP_TIME = 5;
 
  if (becomeDaemon(0) == -1)
  errExit("becomeDaemon");
  

  initialize_monitor_fd_set();

  unlink(SOCKET_NAME);
  // create master_fd socket
  connection_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (connection_socket < 0) {
    perror("socket");
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
    exit(EXIT_FAILURE);
  }
  printf("Socket binding successful \n");

  ret = listen(connection_socket, 20);
  if (ret == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  add_to_monitored_fd(connection_socket);




  logOpen("/tmp/ds.log");
  readConfigFile("/tmp/ds.conf");

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
      // aacept the request

      data_socket = accept(connection_socket, NULL, NULL);
      if (data_socket < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }
      printf("Conenction established for new client\n");
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
  printf("Connection closed\n");
  unlink(SOCKET_NAME);
  exit(EXIT_SUCCESS);
}