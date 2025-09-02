// become_daemon.h
#ifndef BECOME_DAEMON_H
#define BECOME_DAEMON_H


void errExit(const char *msg);
int becomeDaemon(int flags);
void logOpen(const char *logFilename);
void logClose(void);
void logMessage(const char *format, ...);
void readConfigFile(const char *configFilename);
char *getMemInfo(void);
double getCpuUsage(void);
double getCpuTemperature(void);
#endif