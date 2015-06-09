#include <stdlib.h>
#include <string.h>

#include "dir.h"

bson_t* dir_getBSON(dir_t *dir) {
	bson_t *bson = bson_new();
	BSON_APPEND_UTF8(bson, "_id", dir->id);
	BSON_APPEND_UTF8(bson, "name", dir->name);
	BSON_APPEND_UTF8(bson, "parentId", dir->parentId);
	return bson;
}

dir_t* dir_getDirFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	dir_t *dir = dir_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				strcpy(dir->id, value->value.v_utf8.str);
			} else if (strcmp(key, "name") == 0) {
				strcpy(dir->name, value->value.v_utf8.str);
			} else if (strcmp(key, "parentId") == 0) {
				strcpy(dir->parentId, value->value.v_utf8.str);
			}
			/*
			if (bson_iter_find(&iter, "_id")) strcpy(dir->id, bson_iter_utf8(&iter, NULL));
			if (bson_iter_find(&iter, "name")) strcpy(dir->name, bson_iter_utf8(&iter, NULL));
			if (bson_iter_find(&iter, "parentId")) strcpy(dir->parentId, bson_iter_utf8(&iter, NULL));
			 */
		}
	}

	return dir;
}

dir_t* dir_create() {
	dir_t* dir = malloc(sizeof(dir_t));
	dir->name = malloc(sizeof(char) * 512);
	dir->parentId = malloc(ID_SIZE);
	return dir;
}

void dir_free(dir_t* dir) {
	free(dir->name);
	free(dir->parentId);
	free(dir);
}
