
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


# Chapter 63/  I/O Multiplexing

I/O multiplexing allows us to simultaneously monitor multiple file descriptors to
see if I/O is possible on any of them. 

##  The select() System Call
The select() system call blocks until one or more of a set of file descriptors becomes
ready.

```c
#include <sys/time.h> /* For portability */
#include <sys/select.h>
int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
 struct timeval *timeout);
```
Returns number of ready file descriptors, 0 on timeout, or –1 on error

**Parameters**
* nfds: The highest-numbered file descriptor in any of the sets, plus 1.
* readfds: File descriptors to check for read readiness.
* writefds: File descriptors to check for write readiness.
* exceptfds: File descriptors to check for exceptional conditions (e.g., out-of-band data).
* timeout: Specifies how long to wait :
* NULL → wait forever
* 0 → return immediately

This document describes how to use the `select()` system call to create a multiplexed server-client state machine. We'll focus on the maintenance of the `fd_set` data structure which allows for efficient monitoring of multiple file descriptors.

## Table of Contents
- [Introduction](#introduction)
- [State Machine Overview](#state-machine-overview)
- [Understanding `fd_set`](#understanding-fd_set)
- [Steps to Implement](#steps-to-implement)
- [Examples](#examples)
- [Best Practices](#best-practices)
- [References](#references)

## Introduction

Multiplexing in server-client systems involves handling multiple client connections simultaneously without spawning separate threads or processes for each connection. The `select()` system call in UNIX allows us to monitor multiple file descriptors to see if they have data ready for reading or writing.

## State Machine Overview

A multiplexed server-client state machine can be broken down into the following states:

```bash
     +------------------------------------+
     |            SERVER                  |
     |                                    |
     |    +-------------------------+     |
     |    |                         |     |
     |    |       select()          |<-----------------------+
     |    |                         |                        |
     |    +-------------------------+                        |
     |                                    |                  |
     |    +-------------------------+     |                  |
     |    |   Process Ready FDs     |     |                  |
     |    +-------------------------+     |                  |
     +------------------------------------+                  |
            |            |            |                       |
   +--------+     +-----+------+  +----+------+               |
   |        |     |            |  |           |               |
   | Client |     | Client 2   |  | Client N  |               |
   |    1   |     |            |  |           |               |
   +--------+     +------------+  +-----------+               |
      |                |              |                       |
      +------------------------------------------------------+
```

```scss

       Server Side                 Client Side
    .---------.                    .---------.    
   /  socket() \                  /  socket() \   
  (   bind()   )                 (   client1  )  
   \ listen()  /                  \ connect() /    
    '-------'                     '---------'      
        |                             |            
        | fd_set: master_fd           V            
        V                             |            
    .-------.                    .---------.       
   / select() \                  /  socket() \      
  ( fd_set[]  )<---------------->(   client2  )      
   \ accept() /   fd_set: new_fd \ connect() /       
    '-------'                     '---------'       
        |                             |             
        V                             V             
     close()                     ...               

             ... continued for clientN

```

## Explanation:

1. The **Server** listens for incoming client connections.
2. Multiple **Clients** (Client 1, Client 2, ... Client N) connect to the server.
3. The **Server** uses `select()` to monitor the file descriptors (FDs) of all connected clients to check if any of them have data that's ready to be read or processed.
4. When the `select()` call identifies ready FDs, the server then processes them accordingly.
5. Server can send responses back to the clients, and this loop continues, allowing the server to handle multiple clients concurrently without the need for threading or multiprocessing.

Using the `select()` syscall, the server can efficiently handle many clients at once without being blocked by any single client's activity.



## Understanding `fd_set`

`fd_set` is a data structure used to represent a set of file descriptors. It plays a pivotal role in utilizing the `select()` function effectively.

### Key Functions & Macros for `fd_set` Management:

- `FD_ZERO(fd_set *set)`: Initializes the file descriptor set to have zero bits for all file descriptors.
- `FD_SET(int fd, fd_set *set)`: Set the bit for the file descriptor `fd` in the set.
- `FD_CLR(int fd, fd_set *set)`: Clear the bit for the file descriptor `fd` in the set.
- `FD_ISSET(int fd, fd_set *set)`: Test to see if the bit for the file descriptor `fd` is set in the set, returning a non-zero value if true.

### Maintaining `fd_set`:

1. **Initialization**: Always initialize an `fd_set` using `FD_ZERO` before use.
2. **Add New Connections**: As new clients connect, use `FD_SET` to add their file descriptors to the monitoring set.
3. **Remove Disconnected Clients**: When clients disconnect or after processing a client's data, use `FD_CLR` to remove their file descriptors from the set.
4. **Checking Activity**: After `select()` returns, use `FD_ISSET` in a loop to check which file descriptors are "ready" for reading or writing.

## Steps to Implement

1. Create a master `fd_set` and use `FD_ZERO` to initialize it.
2. As clients connect, add their file descriptors using `FD_SET`.
3. Call `select()` to block until any client file descriptor becomes ready.
4. After `select()` returns, iterate through all file descriptors and use `FD_ISSET` to determine which ones are ready.
5. Process ready file descriptors accordingly (read, write, or disconnect).
6. Repeat steps 3-5 as needed.

## Examples

For illustrative code samples that demonstrate using `select()` with the server-client state machine model, consider diving deeper into UNIX networking programming tutorials or textbooks.

## Best Practices

- Always reinitialize your `fd_set` before a new call to `select()`.
- Handle errors from `select()` gracefully, especially `EINTR` which indicates an interrupted system call.
- Ensure you handle the case where `select()` times out without any file descriptor activity.
  
## References

- [man select](http://man7.org/linux/man-pages/man2/select.2.html)
- "UNIX Network Programming" by W. Richard Stevens


# `select()` System Call in UNIX

The `select()` system call is an essential tool in UNIX-based systems for multiplexing input/output operations across multiple file descriptors. This README highlights its significance, particularly in multiplexing UNIX domain client-server interactions.

## Table of Contents
* [Overview](#overview)
* [Limitations and Recommendations](#limitations-and-recommendations)
* [Usage in Multiplexing UNIX Domain Sockets](#usage-in-multiplexing-unix-domain-sockets)
* [Key Functions and Macros](#key-functions-and-macros)
* [Examples](#examples)
* [References](#references)

## Overview

`select()` allows programs to monitor multiple file descriptors, waiting until one or more of them become "ready" for some kind of I/O operation.

## Limitations and Recommendations

**FD_SETSIZE Limitation**: 
The `select()` system call can monitor only file descriptor numbers less than `FD_SETSIZE` (commonly 1024). This can be restrictive for many modern applications.

**Recommendation**: 
Modern applications requiring more scalability should use `poll(2)` or `epoll(7)`.

## Usage in Multiplexing UNIX Domain Sockets

In a UNIX domain client-server scenario, a server might need to handle numerous client connections simultaneously. Using `select()`, the server can keep tabs on all its client sockets, reacting whenever any of the sockets have data ready for reading or are ready for writing.

**Steps**:

1. **Server Initialization**: Set up a UNIX domain socket and start listening for client connections.
2. **Accepting Clients**: Store file descriptors as clients connect.
3. **Multiplexing with `select()`**: Monitor all client sockets at once.
4. **Processing Data**: If `select()` indicates one or more descriptors are ready, process them.
5. **Closing Connections**: Close the respective socket once data exchange with a client concludes.

## Key Functions and Macros

* `FD_ZERO(fd_set *set)`: Clear the file descriptor set.
* `FD_SET(int fd, fd_set *set)`: Add a file descriptor to the set.
* `FD_CLR(int fd, fd_set *set)`: Remove a file descriptor from the set.
* `FD_ISSET(int fd, fd_set *set)`: Check if a file descriptor is in the set.

## Examples

For code samples showing the `select()` function in action with UNIX domain sockets, refer to renowned UNIX networking programming resources or online tutorials.

## References

* [man select](http://man7.org/linux/man-pages/man2/select.2.html)
* "UNIX Network Programming" by W. Richard Stevens
* "Advanced Programming in the UNIX Environment" by W. Richard Stevens

------
# Multiplexed Client-Server Model using `select()` System Call for Sum Calculation

## Overview:

A server is designed to accept connections from multiple clients. Each client sends integers to the server. The server maintains a running sum of integers received from each individual client. When a client sends the integer value `0`, the server responds back with the total sum of integers sent by that client up to that point and closes the connection to that client.

## Problem Diagram:

```sql

         +----------------------------------+
         |            SERVER                |
         |                                  |
         |    +-----------------------+     |
         |    |      select()         |<--------------------+
         |    +-----------------------+     |               |
         |                                  |               |
         |    +-----------------------+     |               |
         |    |  Calculate & Respond  |     |               |
         |    +-----------------------+     |               |
         +--------+----+----------+---------+               |
         | Init FD | Add| Refresh | Remove  |               |
         +--------+----+----------+---------+               |
              |          |          |                       |
     +--------+   +------+-----+  +----+-----+               |
     |        |   |            |  |         |               |
     | Client |   | Client 2   |  | Client N|               |
     |    1   |   |            |  |         |               |
     +--------+   +------------+  +---------+               |
        |              |            |                       |
        +--------------------------------------------------+
```

## Design:

1. **Server Initialization**: 
   - The server initializes and binds to a known UNIX domain address.
   - It starts listening for incoming client connections.
   - Initializes arrays for monitored file descriptors and client results.

2. **Client Connection**: 
   - Each client connects to the server.
   - Server, on accepting a new client connection, adds the client's file descriptor to the monitored set.

3. **Data Transmission**:
   - Clients send integers to the server.
   - Server, on receiving an integer from a client:
     - If the integer is `0`, responds back with the sum for that client, resets the client's sum to zero, and closes the client connection.
     - If the integer is not `0`, the server adds the integer to that client's running total.

4. **Server Multiplexing**:
   - The server uses `select()` to efficiently manage and monitor file descriptors of all connected clients without blocking on any individual client's activity. It also refreshes the FD set before each call to `select()`.
   
5. **Utilities**:
   - The server has utility functions to initialize, add, remove, and refresh the monitored FD set. It also has a utility function to get the maximum FD for use with `select()`.

## Pseudocode:

```
initialize monitored FD set
create server socket
bind and listen on server socket
add server socket to monitored FD set

while True:
    refresh FD set for select()
    use select() to wait for activity

    if there's activity on the server socket:
        accept new client connection
        add new client's FD to monitored set

    else:
        for each client FD in monitored set:
            if there's activity on client FD:
                read integer from client

                if integer == 0:
                    send accumulated sum to client
                    close client connection
                    reset client's sum to zero
                    remove client FD from monitored set

                else:
                    add integer to client's accumulated sum

```
## Expexted Output 
![](./Screenshot%20from%202023-08-18%2023-51-27.png)


## References
---
This project includes code adapted from the following repository:

- https://github.com/ANSANJAY/unix-domain-mux-state-machine


---

**Author: Alperen ÖZTAŞ**

---