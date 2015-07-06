#ifndef CONNECTIONS_NODE_H
#define CONNECTIONS_NODE_H

#include "connections.h"

void connections_node_initialize();
void connections_node_shutdown();

typedef struct {
	char *nodeId;
	char *ip;
	uint16_t port;
	char *tmpFileName;
} node_connection_getTmpFileOperation_t;

void* connections_node_accept(void *param);
char* connections_node_getFileContent(node_connection_getTmpFileOperation_t *operation);

node_connection_getTmpFileOperation_t* node_connection_getTmpFileOperation_create(char *nodeId, char *ip, uint16_t port, char *tmpFileName);
void node_connection_getTmpFileOperation_free(node_connection_getTmpFileOperation_t *operation);

#endif
