#ifndef CONNECTIONS_JOB_H
#define CONNECTIONS_JOB_H

#include "connections.h"
#include <time.h>

void connections_job_initialize();
void connections_job_shutdown();

void* connections_job_accept(void *param);

#endif
