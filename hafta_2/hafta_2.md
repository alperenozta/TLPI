
---

# CHAPTER 56/ SOCKETS: INTRODUCTION

## Socket system calls

The key socket system calls are the following:

* The `socket()` system call creates a new socket.
* The `bind()` system call binds a socket to an address. Usually, a server employs this call to bind its socket to a well-known address so that clients can locate the socket.
* The `listen()` system call allows a stream socket to accept incoming connections from other sockets.
* The `accept()` system call accepts a connection from a peer application on a listening stream socket, and optionally returns the address of the peer socket.
* The `connect()` system call establishes a connection with another socket.

---

## Creating Socket

`socket()` system call creates a new socket.

```c
#include <sys/socket.h>
int socket(int domain, int type, int protocol);
```

* The `domain` argument specifies the communication domain for the socket.
* The `type` argument specifies the socket type.

---

## Binding a Socket to an Address: `bind()`

The `bind()` system call binds a socket to an address.

```c
#include <sys/socket.h>
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

* The **sockfd** argument is a file descriptor obtained from a previous call to **socket()**.
* The **addr** argument is a pointer to a structure specifying the address to which this socket is to be bound. The type of structure passed in this argument depends on the socket domain.
* The **addrlen** argument specifies the size of the address structure.
* The **socklen\_t** data type used for the addrlen argument is an integer type specified by SUSv3.

---

### Generic Socket Address Structures: `struct sockaddr`

Calls such as `bind()` are generic to all socket domains, they must be able to accept address structures of any type. In order to permit this, the sockets API defines a generic address structure, `struct sockaddr`. The only purpose for this type is to cast the various domain-specific address structures to a single type for use as arguments in the socket system calls. The `sockaddr` structure is typically defined as follows:

```c
struct sockaddr {
    sa_family_t sa_family;    /* Address family (AF_* constant) */
    char sa_data[14];         /* Socket address (size varies according to socket domain) */
};
```

---

### Stream Sockets – Operation Steps

1. **socket()**

   * Creates a socket for communication.
   * Each application must create its own socket.

2. **bind()**

   * Assigns a local address (IP and port) to the socket.
   * Used by the server side.

3. **listen()**

   * Marks the socket as passive and ready to accept incoming connections.

4. **connect()**

   * Initiated by the client.
   * Connects to the server's socket using its address.

5. **accept()**

   * Used by the server.
   * Accepts the connection request from a client.
   * If no client is attempting to connect, this call blocks.

6. **read() / write() or recv() / send()**

   * Used after connection is established.
   * Enables bi-directional data transfer between sockets.

7. **close()**

   * Closes the connection.
   * Either side can initiate connection termination.

---

## Listening for Incoming Connections: `listen()`

The socket will subsequently be used to accept connections from other (active) sockets.

```c
#include <sys/socket.h>
int listen(int sockfd, int backlog);
```

Returns `0` on success, or `–1` on error.

We can’t apply `listen()` to a connected socket — that is, a socket on which a `connect()` has been successfully performed or a socket returned by a call to `accept()`.

---

## Accepting a Connection: `accept()`

```c
#include <sys/socket.h>
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

Returns file descriptor on success, or `–1` on error.

---

## Connecting to a Peer Socket: `connect()`

The `connect()` system call connects the active socket referred to by the file descriptor `sockfd` to the listening socket whose address is specified by `addr` and `addrlen`.

```c
#include <sys/socket.h>
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
```

Returns `0` on success, or `–1` on error.

If `connect()` fails and we wish to reattempt, portable method of doing so is to close the socket, create a new socket, and reattempt the connection with the new socket.

---

### Datagram Sockets

1. `socket()` creates a datagram socket, like setting up a personal mailbox.
2. `bind()` assigns an address to the socket, allowing others to send data to it.
3. `sendto()` sends a message to a specific address, like mailing a letter.
4. `recvfrom()` receives a message and the sender's address.
5. `close()` closes the socket when done.

Datagram messages may arrive out of order, be duplicated, or never arrive — just like unreliable mail.

---

## Exchanging Datagrams: `recvfrom()` and `sendto()`

```c
#include <sys/socket.h>

ssize_t recvfrom(int sockfd, void *buffer, size_t length, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen);
```

Returns number of bytes received, 0 on EOF, or `–1` on error.

```c
ssize_t sendto(int sockfd, const void *buffer, size_t length, int flags,
               const struct sockaddr *dest_addr, socklen_t addrlen);
```

Returns number of bytes sent, or `–1` on error.

---

# CHAPTER 57/ SOCKETS: UNIX DOMAIN

UNIX domain sockets allow **interprocess communication** (IPC) between processes **on the same host**. These sockets are represented by **filesystem pathnames**, enabling secure and efficient communication between local processes.

---

### `sockaddr_un` Structure

```c
struct sockaddr_un {
    sa_family_t sun_family;       /* Always AF_UNIX */
    char sun_path[108];           /* Null-terminated socket pathname */
};
```

* `sun_family`: Specifies the socket family — always `AF_UNIX` for UNIX domain sockets.
* `sun_path`: The path in the filesystem that identifies the socket (e.g., `/tmp/mysock`).
* `memset()` is used to clear the structure to ensure all fields (including implementation-specific ones) are initialized to zero.
* `strncpy()` safely copies the pathname into the `sun_path` field.
* `bind()` associates the socket with a file system path.
* A socket cannot be bound to an existing pathname (returns `EADDRINUSE`).
* When the socket is no longer needed, its file should be removed with `unlink()`.
* Using `/tmp` is common but potentially insecure due to public write permissions.

---

## Code: Creating and Binding a UNIX Domain Socket

```c
const char *SOCKNAME = "/tmp/mysock";
int sfd;
struct sockaddr_un addr;

sfd = socket(AF_UNIX, SOCK_STREAM, 0); /* Create socket */
if (sfd == -1)
    errExit("socket");

memset(&addr, 0, sizeof(struct sockaddr_un)); /* Zero out the structure */
addr.sun_family = AF_UNIX; /* Set address family */
strncpy(addr.sun_path, SOCKNAME, sizeof(addr.sun_path) - 1); /* Copy path */

if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
    errExit("bind");
```

---

## Code Explanation

| Line              | Explanation                                                                          |
| ----------------- | ------------------------------------------------------------------------------------ |
| `SOCKNAME`        | Defines the socket's pathname in the filesystem (`/tmp/mysock`).                     |
| `socket()`        | Creates a UNIX domain stream socket (`SOCK_STREAM`, similar to TCP).                 |
| `memset()`        | Clears the memory of the `addr` structure for safe initialization.                   |
| `addr.sun_family` | Sets the address family to `AF_UNIX`.                                                |
| `strncpy()`       | Copies the socket path into the structure, preventing buffer overflow.               |
| `bind()`          | Binds the socket to the given path; fails if the path already exists (`EADDRINUSE`). |

---

* The `/tmp` directory is world-writable, making it vulnerable to **Denial of Service (DoS)** attacks.
* Real-world applications should bind UNIX domain sockets in secure, private directories with restricted access.

---
