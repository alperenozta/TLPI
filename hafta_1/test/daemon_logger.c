
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "become_daemon.h"


static const char *LOG_FILE = "/tmp/ds.log";
static const char *CONFIG_FILE = "/tmp/ds.conf";

static FILE *logfp;                 /* Log file stream */

void
logMessage(const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    vfprintf(logfp, format, argList);
    fprintf(logfp, "\n");
    va_end(argList);
}

void
logOpen(const char *logFilename)
{
    mode_t m;

    m = umask(077);
    logfp = fopen(logFilename, "a");
    umask(m);

    /* If opening the log fails we can't display a message... */

    if (logfp == NULL)
        exit(EXIT_FAILURE);

    setbuf(logfp, NULL);                    /* Disable stdio buffering */

    logMessage("Opened log file");
}

/* Close the log file */

void
logClose(void)
{
    logMessage("Closing log file");
    fclose(logfp);
}


void
readConfigFile(const char *configFilename)
{
    FILE *configfp;
#define SBUF_SIZE 100
    char str[SBUF_SIZE];

    configfp = fopen(configFilename, "r");
    if (configfp != NULL) {                 /* Ignore nonexistent file */
        if (fgets(str, SBUF_SIZE, configfp) == NULL)
            str[0] = '\0';
        else
            str[strlen(str) - 1] = '\0';    /* Strip trailing '\n' */
        logMessage("Read config file: %s", str);
        fclose(configfp);
    }
}

void errExit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

char* getMemInfo(){
    static char result[1024];
    result[0] = '\0';

    FILE *fp = fopen("/proc/meminfo","r");
    if (fp == NULL){
        perror("meminfo couldn't be opened");
        return NULL;
    }

    int found = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0 ||
            strncmp(line, "MemFree:", 8) == 0 ||
            strncmp(line, "MemAvailable:", 13) == 0) {

            if (strlen(result) + strlen(line) < sizeof(result)) {
                strcat(result, line);
                found++;
            }
        }

        if (found == 3)
            break;
    }

    fclose(fp);
    if (found < 3)
        return NULL;

    FILE *sp = fopen("/proc/stat", "r");
    if (sp == NULL){
        perror("stat couldn't be opened");
        return NULL;
    }

    while (fgets(line, sizeof(line), sp)) {
        if (strlen(result) + strlen(line) < sizeof(result)) {
            strcat(result, line);
        } else {
            break;
        }
    }

    fclose(sp);
    return result;
}


itn
main(int argc, char *argv[])
{
    const int SLEEP_TIME = 5;      /* Time to sleep between messages */
    int count = 0;                  /* Number of completed SLEEP_TIME intervals */
    int unslept;  

if (becomeDaemon(0) == -1)
    errExit("becomeDaemon");
    


logOpen(LOG_FILE);
readConfigFile(CONFIG_FILE);

unslept = SLEEP_TIME;
logMessage("MEMORY INFO");
for (;;) {
    unslept = sleep(unslept); 

    if (unslept == 0) {             /* On completed interval */
        count++;
        char *memLine = getMemInfo();
        if (memLine != NULL) {
        logMessage("%s", memLine);
        } else {
        logMessage("MemTotal info not found.\n");
        }
        
        unslept = SLEEP_TIME;       /* Reset interval */
        }
    }
}
