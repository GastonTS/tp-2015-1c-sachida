#ifndef FILE_H
#define FILE_H

#include "../mongo/mongo.h"

typedef struct {
	char id[25];
	char name[100];
	int32_t size;
	char parentId[25];
} file_t;

bson_t* file_getBSON(file_t *file);
file_t* file_getFileFromBSON(const bson_t *doc);

#endif
