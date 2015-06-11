#ifndef FILE_H
#define FILE_H

#include "../mongo/mongo.h"

typedef struct {
	char id[ID_SIZE];
	char parentId[ID_SIZE];
	char *name;
	size_t size;
	t_list *blocks; // (list of blocks with the list of the copies)
} file_t;

typedef struct {
	char nodeId[ID_SIZE];
	off_t blockIndex;
} file_block_t;

bson_t* file_getBSON(file_t *file);
file_t* file_getFileFromBSON(const bson_t *doc);

file_t* file_create();
void file_free(file_t* file);

file_block_t* file_block_create();

#endif
