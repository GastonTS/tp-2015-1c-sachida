#ifndef MONGO_NODE_H
#define MONGO_NODE_H

#include "../structs/node.h"

bool mongo_node_init();
void mongo_node_shutdown();

bool mongo_node_save(node_t *node);

node_t* mongo_node_getById(char id[25]);

bool mongo_node_deleteAll();


#endif
