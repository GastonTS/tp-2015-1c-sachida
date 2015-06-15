#ifndef NODE_H
#define NODE_H

#include <commons/bitarray.h>
#include "../mongo/mongo.h"

typedef struct {
	char *id;
	t_bitarray *blocks;
	uint16_t blocksCount;
} node_t;

bson_t* node_getBSON(node_t *node);
node_t* node_getNodeFromBSON(const bson_t *doc);

void node_setAllBlocksFree(node_t *node);
void node_setBlockUsed(node_t* node, off_t blockIndex);
void node_setBlockFree(node_t* node, off_t blockIndex);
bool node_isBlockUsed(node_t* node, off_t blockIndex);
int node_getBlocksFreeCount(node_t *node);
off_t node_getFirstFreeBlock(node_t *node);

node_t* node_create(uint16_t blocksCount);
void node_free(node_t* node);

void node_printBlocksStatus(node_t *node);

#endif
