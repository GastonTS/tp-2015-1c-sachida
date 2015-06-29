#include "tlog.h"

t_log* tlog_create(char* file, char *program_name, bool is_active_console, t_log_level level) {
	pthread_mutex_init(&loggerLock, NULL);
	return log_create(file, program_name, is_active_console, level);
}

void tlog_destroy(t_log* logger) {
	pthread_mutex_destroy(&loggerLock);
	log_destroy(logger);
}

