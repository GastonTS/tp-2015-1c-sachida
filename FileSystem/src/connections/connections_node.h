#ifndef CONNECTIONS_NODE_H
#define CONNECTIONS_NODE_H

#include <stdio.h>
#include <stdlib.h>
#include "../filesystem/filesystem.h"

typedef struct {
	int socket;
	char *ip;
	uint16_t listenPort;
} node_connection_t;

node_connection_t* connections_node_connection_create(int socket, char *ip);
void connections_node_connection_free(node_connection_t *nodeConnection);

node_connection_t* connections_node_getActiveNodeConnection(char *nodeId);
int connections_node_getActiveConnectedCount();
bool connections_node_activateNode(char *nodeId);
bool connections_node_deactivateNode(char *nodeId);
bool connections_node_isActiveNode(char *nodeId);

void connections_node_initialize();
void connections_node_shutdown();

void* connections_node_accept(void *param);
bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation);
char* connections_node_getBlock(file_block_t *fileBlock);
char* connections_node_getFileContent(char *nodeId, char *tmpFileName, size_t *tmpFileLength);

#endif
