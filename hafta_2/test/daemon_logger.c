
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "become_daemon.h"



static FILE *logfp;                 /* Log file stream */

typedef struct {
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
} cpu_times_t;

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

    static char result[512];
    result[0] = '\0';

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("meminfo couldn't be opened");
        return NULL;
    }

    const struct {
        const char *key;
        int        key_len;
    } wanted[] = {
        { "MemTotal:",     9 },
        { "MemFree:",      8 },
        { "MemAvailable:", 13 },
    };

    int found = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp) && found < 3) {
        for (int i = 0; i < 3; i++) {
            if (strncmp(line, wanted[i].key, wanted[i].key_len) == 0) {
                unsigned long kb;
                // Satırdan sayıyı oku (örneğin "  16204880 kB")
                if (sscanf(line + wanted[i].key_len, "%lu", &kb) == 1) {
                    double mb = (double)kb / 1024.0;
                    char buf[64];
                    // İki ondalık basamak gösterelim
                    snprintf(buf, sizeof(buf),
                             "%-13s %.2f MB\n",
                             wanted[i].key, mb);
                    // result için taşma kontrolü
                    if (strlen(result) + strlen(buf) < sizeof(result)) {
                        strcat(result, buf);
                    }
                    found++;
                }
                break;
            }
        }
    }

    fclose(fp);

    if (found < 3)
        return NULL;

    return result;
}

double getCpuUsage(void) {
    static cpu_times_t prev = {0}, curr;
    static int first = 1;
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("fopen /proc/stat");
        return 0.0;
    }

    // Sadece ilk satır ("cpu ...")  
    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return 0.0;
    }
    fclose(fp);

    // Parçala
    sscanf(line, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu",
           &curr.user, &curr.nice, &curr.system, &curr.idle,
           &curr.iowait, &curr.irq, &curr.softirq, &curr.steal);

    if (first) {
        prev = curr;
        first = 0;
        return 0.0;
    }

    // Toplam ve boşta geçen zaman farkları
    unsigned long long prev_idle = prev.idle + prev.iowait;
    unsigned long long curr_idle = curr.idle + curr.iowait;

    unsigned long long prev_non_idle = prev.user + prev.nice + prev.system
        + prev.irq + prev.softirq + prev.steal;
    unsigned long long curr_non_idle = curr.user + curr.nice + curr.system
        + curr.irq + curr.softirq + curr.steal;

    unsigned long long prev_total = prev_idle + prev_non_idle;
    unsigned long long curr_total = curr_idle + curr_non_idle;

    unsigned long long totald = curr_total - prev_total;
    unsigned long long idled  = curr_idle  - prev_idle;

    prev = curr;

    if (totald == 0) return 0.0;
    return (double)(totald - idled) * 100.0 / (double)totald;
}

double getCpuTemperature(void) {
    FILE *fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) {
        perror("Failed to open thermal_zone0/temp");
        return -1.0;
    }

    long millideg;
    if (fscanf(fp, "%ld", &millideg) != 1) {
        perror("Failed to read temperature");
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    return millideg / 1000.0;  // millidegree → degree
}

