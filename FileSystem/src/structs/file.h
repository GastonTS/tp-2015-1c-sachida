#ifndef FILE_H
#define FILE_H

#include "../mongo/mongo.h"

typedef struct {
	char id[ID_SIZE];
	char *name;
	char *parentId;
	int32_t size;
} file_t;

bson_t* file_getBSON(file_t *file);
file_t* file_getFileFromBSON(const bson_t *doc);

file_t* file_create();
void file_free(file_t* file);

#endif
