```markdown
# UNIX-Domain Multiplexing Server Daemon

This C program implements a UNIX-domain socket server that runs as a daemon. It concurrently:

1. Accepts commands from multiple clients over a single socket using `select()`.  
2. Responds to each client’s command and then closes that client’s connection.  
3. Every 5 seconds, logs system statistics (CPU temperature, CPU usage %, memory info) to a rolling log file.

---

## 🔧 Features

- **Daemonized** via `becomeDaemon()`  
- **Multiplexed I/O** with `select()` and an `fd_set` to serve multiple clients in one thread  
- **Per-client commands**:
  - `show mem`  → total/free/available memory (in kB)  
  - `show cpu`  → current CPU usage %  
  - `show temp` → current CPU temperature (°C)  
  - `loginfo`   → “Log level INFO set” confirmation  
  - `help`      → list of supported commands  
- **Periodic logging** every 5 seconds:
  - CPU temperature  
  - CPU usage %  
  - Memory totals (MemTotal, MemFree, MemAvailable)  

---

## Project Layout

```

.
├── become\_daemon.h       # daemon helper API
├── daemon\_logger.c       # log + periodic stats routines
├── declarations.h        # shared constants (e.g. SOCKET\_NAME, BUFFER\_SIZE)
├── headers.h             # additional shared prototypes
├── server.c              # main() for the multiplexing daemon
├── client.c              # simple client for testing
├── Makefile              # build rules
└── README.md             # this file

````

---

## Building

Requires a POSIX-compliant system (Linux).

```bash
make
````

This produces four binaries:

* `server`            → the daemonized multiplexing server
* `client`            → a simple client to send commands
* `test_become_daemon`→ TLPI test harness for `becomeDaemon()`
* `daemon_logger`     → standalone logger/test for the logging library

To clean up:

```bash
make clean
```

---

## Running the Server

```bash
./server
```

* The server will daemonize and detach from your terminal.
* It will create the UNIX-domain socket at the path defined in `declarations.h` (e.g. `/tmp/mysocket`).
* Every 5 seconds it will append CPU+memory+temp stats to `/tmp/ds.log`.

---

## Running the Client

```bash
./client
```

1. Connects to the server’s UNIX socket.
2. Prompts you for a command.
3. Sends the command and prints the server’s response.
4. Type `exit` to close the client.

---

## Configuration & Logs

* **Log file**: `/tmp/ds.log`
* **Config file** (optional): `/tmp/ds.conf`

  * If present at startup, its single line is read and recorded in the log.

---

## Stopping the Daemon

Find the server’s PID and send `SIGTERM` (or use `pkill`):

```bash
pkill server
```

If it does not exit quickly, force-kill with:

```bash
pkill -9 server
```

