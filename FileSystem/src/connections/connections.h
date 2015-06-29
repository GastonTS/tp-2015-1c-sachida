#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
//#include <semaphore.h>
#include <commons/log.h>
#include "../utils/socket.h"

typedef struct {
	int port;
	int minNodesCount;
} fs_connections_cfg_t;

void connections_initialize(fs_connections_cfg_t *config);
void connections_shutdown();

#endif
