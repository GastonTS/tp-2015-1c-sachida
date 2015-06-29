#ifndef SRC_UTILS_LOG_H_
#define SRC_UTILS_LOG_H_

#include <commons/log.h>
#include <pthread.h>

pthread_mutex_t loggerLock;

t_log* tlog_create(char* file, char *program_name, bool is_active_console, t_log_level level);
void tlog_destroy(t_log* logger);

#define tlog_trace(logger, message_template, ...) do {     \
    pthread_mutex_lock(&loggerLock);                       \
    log_trace(logger, message_template, ##__VA_ARGS__);    \
    pthread_mutex_unlock(&loggerLock);                     \
} while(0)

#define tlog_debug(logger, message_template, ...) do {     \
    pthread_mutex_lock(&loggerLock);                       \
    log_debug(logger, message_template, ##__VA_ARGS__);    \
    pthread_mutex_unlock(&loggerLock);                     \
} while(0)

#define tlog_info(logger, message_template, ...) do {      \
    pthread_mutex_lock(&loggerLock);                       \
    log_info(logger, message_template, ##__VA_ARGS__);     \
    pthread_mutex_unlock(&loggerLock);                     \
} while(0)

#define tlog_warning(logger, message_template, ...) do {   \
    pthread_mutex_lock(&loggerLock);                       \
    log_warning(logger, message_template, ##__VA_ARGS__);  \
    pthread_mutex_unlock(&loggerLock);                     \
} while(0)

#define tlog_error(logger, message_template, ...) do {     \
    pthread_mutex_lock(&loggerLock);                       \
    log_error(logger, message_template, ##__VA_ARGS__);    \
    pthread_mutex_unlock(&loggerLock);                     \
} while(0)

/*
 void tlog_trace(t_log* logger, const char* message, ...);
 void tlog_debug(t_log* logger, const char* message, ...);
 void tlog_info(t_log* logger, const char* message, ...);
 void tlog_warning(t_log* logger, const char* message, ...);
 void tlog_error(t_log* logger, const char* message, ...);
 */

#endif
