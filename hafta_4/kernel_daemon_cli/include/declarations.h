#define SOCKET_NAME "/tmp/DemoSocket"
#define BUFFER_SIZE 256
#define MAX_CLIENT_SUPPORTED 32

typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARNING = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} log_level_t;

extern log_level_t current_log_level;  // sadece deklarasyon
void log_level(log_level_t level, const char *fmt, ...);



