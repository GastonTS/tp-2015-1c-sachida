#ifndef NODE_H
#define NODE_H

#include <commons/bitarray.h>
#include "../mongo/mongo.h"

#define	NODE_BLOCK_SIZE	20 * 1024 * 1024

typedef struct {
	char id[ID_SIZE];
	char *name;
	t_bitarray *blocks;
	int *blocksCount;
} node_t;

bson_t* node_getBSON(node_t *node);
node_t* node_getNodeFromBSON(const bson_t *doc);

void node_setBlockUsed(node_t* node, int blockIndex);
void node_setBlockFree(node_t* node, int blockIndex);
bool node_isBlockUsed(node_t* node, int blockIndex);
int node_getBlocksFreeCount(node_t *node);
int node_getFirstFreeBlock(node_t *node);

node_t* node_create(int blocksCount);
void node_free(node_t* node);

void node_printBlocksStatus(node_t *node);

#endif
