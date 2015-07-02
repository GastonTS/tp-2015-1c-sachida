#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utils/socket.h"
#include <commons/log.h> // log_create, log_info, log_error

void startJob();
void job_enviarAcciones(int nodeSocket);

t_log* logger;

