#ifndef CONNECTIONS_NODE_H
#define CONNECTIONS_NODE_H

#include "connections.h"

void connections_node_initialize();
void connections_node_shutdown();

void* connections_node_accept(void *param);

#endif
