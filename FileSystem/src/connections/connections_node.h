#ifndef CONNECTIONS_NODE_H
#define CONNECTIONS_NODE_H

#include <stdio.h>
#include <stdlib.h>
#include "../filesystem/filesystem.h"

#define NODE_COMMAND_SET_BLOCK 1
#define NODE_COMMAND_GET_BLOCK 2

void connections_node_initialize();
void connections_node_shutdown();

void connections_node_accept(int socketAccepted, char *clientIP);
bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation);
char* connections_node_getBlock(file_block_t *fileBlock);

#endif
