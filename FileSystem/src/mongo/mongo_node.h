#ifndef MONGO_NODE_H
#define MONGO_NODE_H

#include "../structs/node.h"

void mongo_node_init();
void mongo_node_shutdown();

bool mongo_node_save(node_t *node);

node_t* mongo_node_getById(char *id);
t_list* mongo_node_getAll();

bool mongo_node_deleteAll();

void mongo_node_updateBlocks(node_t *node);

#endif
