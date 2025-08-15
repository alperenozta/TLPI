

# Daemon Logger: Memory and CPU stats logging tool

This C program runs as a daemon (background process) and periodically logs system memory and CPU statistics from `/proc/meminfo` and `/proc/stat` into a log file located at `/tmp/ds.log`.

## Features 

- Runs as a daemon
- Every 5 second it logs;
    - `MemTotal`
    - `MemFree`
    - `MemAvailable`
    - `/proc/stat`
- Log file: `/tmp/ds.log`
-Optional configuration file: `/tmp/ds.conf` 

## Setup 

### Create Log file

```bash
echo "Log file created > /tmp/ds.log
```
this command creates new log file if there is not exist otherwise it deletes the content and prints `Log file created`
### Compilation

```bash
make
````

or manuel;

```bash
gcc -Wall -g -o daemon_logger daemon_logger.c become_daemon.c
```
## Usage

```bash
./daemon_logger
```

if it runs succesfully it will be working as a daemon
You can control that by using below script in your terminal

```bash
ps aux | grep daemon_logger
```

Output should be;
```bash
$ ps aux | grep daemon_logger
alperen    34459  0.0  0.0   2496    76 ?        S    14:18   0:00 ./daemon_logger
alperen    34461  0.0  0.0  11780  2808 pts/2    S+   14:18   0:00 grep --color=auto daemon_logger
```
first parameter is < pid >
`?` means it is a daemon because there is not used any terminal for that process

To see log file  

```bash 
tail -f /tmp/ds.log
```

## Terminating

You can use the `kill` command to stop daemon process.

```bash
pkill daemon_logger
```

or

```bash
kill <PID>
```

## Errors and Warnings

* if `/proc/meminfo` or `/proc/stat` don't open program gives error over the `stderr`.
* If the configuration file `/tmp/ds.conf` is missing, the program continues without error.

## Configuration File (Optional)
If the file /tmp/ds.conf exists, its contents are logged during startup.

```bash
echo "daemon_logger active" > /tmp/ds.conf
```
---
**Author:** Alperen ÖZTAŞ

---