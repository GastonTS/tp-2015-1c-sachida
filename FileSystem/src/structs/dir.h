#ifndef DIR_H
#define DIR_H

#include "../mongo/mongo.h"

#define	ROOT_DIR_ID	"0"

typedef struct {
	char id[25];
	char name[100];
	char parentId[25];
} dir_t;


bson_t* dir_getBSON(dir_t *dir);
dir_t* dir_getDirFromBSON(const bson_t *doc);

#endif
