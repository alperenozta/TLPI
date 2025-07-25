
#ifndef US_XFR_H
#define US_XFR_H

#include <sys/un.h>
#include <sys/socket.h>

#define SV_SOCK_PATH "/tmp/us_xfr" // UNIX domain socket path
#define BUF_SIZE 100               // Size of buffer used for data exchange

#endif // US_XFR_H