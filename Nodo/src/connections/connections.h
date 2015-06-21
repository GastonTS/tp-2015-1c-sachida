#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>
#include "../utils/socket.h"
#include "../Nodo.h"

void connections_initialize(t_nodeCfg *nodeCfg);
void connections_shutdown();

#endif
