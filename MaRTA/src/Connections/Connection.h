#ifndef SRC_CONNECTIONS_CONNECTION_H_
#define SRC_CONNECTIONS_CONNECTION_H_

#include "../MaRTA.h"
#include "../../../utils/socket.h"
#include "FSConnection.h"
#include "JobConnection.h"
#include "../structs/job.h"

void initConnection();
int requestFileBlocks(t_file *file);

#endif
