```markdown
# UNIX-Domain-Socket Daemon + CLI Client

## Compilation

Compile both the daemon and client with:

```bash
make
````

* `-Wall` enables all compiler warnings.

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

