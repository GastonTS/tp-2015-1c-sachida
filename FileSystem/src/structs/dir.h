#ifndef DIR_H
#define DIR_H

#include "../mongo/mongo.h"

#define	ROOT_DIR_ID	"0"

typedef struct {
	char id[ID_SIZE];
	char *name;
	char *parentId;
} dir_t;


bson_t* dir_getBSON(dir_t *dir);
dir_t* dir_getDirFromBSON(const bson_t *doc);

dir_t* dir_create();
void dir_free(dir_t* dir);

#endif
