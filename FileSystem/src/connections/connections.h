#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include "../filesystem/filesystem.h"

void connections_initialize(int port);
void connections_shutdown();

bool connections_sendBlockToNode(nodeBlockSendOperation_t *sendOperation);
char* connections_getBlockFromNode(file_block_t *fileBlock);

#endif
