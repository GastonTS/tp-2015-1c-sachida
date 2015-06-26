#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <commons/log.h>

#include "../node.h"
#include "../utils/socket.h"

void connections_initialize();
void connections_shutdown();

#endif
