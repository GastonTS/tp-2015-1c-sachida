#ifndef SRC_CONNECTIONS_FSCONNECTION_H_
#define SRC_CONNECTIONS_FSCONNECTION_H_

#include "../structs/job.h"

void initFSConnection();
int requestFileBlocks(t_file *file);
bool copyFinalTemporal(t_job *job);

#endif
