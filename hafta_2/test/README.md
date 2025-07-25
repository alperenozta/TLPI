
# UNIX-Domain-Socket Daemon + CLI Client

## Features

* The program calls `becomeDaemon()` to detach itself and run in the background as a daemon.
* It creates a UNIX-domain socket (`socket()`), binds it to a filesystem path (e.g. `/tmp/ds.sock`), and enters listening mode with `listen()`.
* A fixed-size array `monitored_fd_set[]` tracks which file descriptors (FDs) are currently being monitored; helpers add new FDs or remove closed ones.
* In an infinite loop, it:

  1. Calls `refresh_fd_set(&readfds)` to populate an `fd_set` with all active FDs.
  2. Uses `select(get_max_fd() + 1, &readfds, NULL, NULL, &tv)` with a timeout (e.g. 5 seconds) to wait for activity.
* After `select()` returns:

  * If `ready == 0`, the timeout expired → the daemon logs CPU usage, temperature, and memory info to `/tmp/ds.log`.
  * If `FD_ISSET(connection_socket, &readfds)` is true → a **new client** is waiting → `accept()` it and add its FD to the monitored set.
  * Otherwise → one of the existing client FDs is ready → `read()` the command, call `handle_and_respond_command()` to generate and `write()` back the response, then close that client FD and remove it from monitoring.
* If `select()` returns `-1` with `errno == EINTR`, it simply retries; other errors break the loop.
* On shutdown, the daemon closes all sockets, removes (`unlink()`) the socket file, and exits cleanly.



## Compilation

Compile both the daemon and client with:

```bash
make
````

---

## Usage

1. **Start the daemon and server**

   ```bash
   ./server
   ```

2. **Run the CLI client**
   Open another terminal and execute:

   ```bash
   ./client
   ```

   At the prompt, enter one of the supported commands:

   * `show cpu`
     Display current CPU usage percentage.
   * `show mem`
     Show the first three lines from `/proc/meminfo`.
   * `show temp`
     Retrieve the CPU temperature.
   * `loginfo`
     Set the daemon’s log level to INFO.
   * `help`
     List all supported commands.

3. **Stop the daemon**
   From any terminal, you can terminate the daemon by:

   ```bash
   pkill daemon
   ```
---

## Details

* **I/O Multiplexing with `select()`**
  The daemon uses `select()` to monitor multiple file descriptors (the listening socket and connected clients) in a single loop.

* **Periodic Logging**
  If no client requests arrive within the configured interval (default 5 seconds), the daemon logs CPU usage, temperature, and memory info to `/tmp/ds.log`.

* **Helper Module (`daemon_logger.c`)**

  * `getCpuUsage()`, `getCpuTemperature()`, `getMemInfo()` read system files and return formatted data.
  * `handle_command()` parses incoming command strings and generates appropriate responses or error messages.

* **Configuration & Logging**

  * Log file: `/tmp/ds.log`
  * Configuration file (optional): `/tmp/ds.conf`
