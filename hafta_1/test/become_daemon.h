// become_daemon.h
#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H



int becomeDaemon(int flags);
void logOpen(const char *logFilename);
void logClose(void);
void logMessage(const char *format, ...);
void readConfigFile(const char *configFilename);
char *getMemInfo(void);
#endif