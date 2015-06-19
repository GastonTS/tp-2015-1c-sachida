#ifndef CONNECTIONS_NODE_H
#define CONNECTIONS_NODE_H

#include <stdio.h>
#include <stdlib.h>
#include "../filesystem/filesystem.h"

#define NODE_COMMAND_SET_BLOCK 1
#define NODE_COMMAND_GET_BLOCK 2

int connections_node_getActiveConnectedCount();
void connections_node_activateNode(char *nodeId);
void connections_node_deactivateNode(char *nodeId);
bool connections_node_isActiveNode(char *nodeId);

void connections_node_initialize();
void connections_node_shutdown();

void connections_node_accept(int socketAccepted, char *clientIP);
bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation);
char* connections_node_getBlock(file_block_t *fileBlock);

#endif
