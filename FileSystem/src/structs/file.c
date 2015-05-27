#include <stdlib.h>
#include <string.h>

#include "file.h"

bson_t* file_getBSON(file_t *file) {

	mongo_generateId(file->id);
	return BCON_NEW(
			"_id", BCON_UTF8(file->id),
			"name", BCON_UTF8(file->name),
			"size", BCON_INT32(file->size),
			"parentId", BCON_UTF8(file->parentId)
		);// TODO change to the other method..
}

file_t* file_getFileFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value;
	const char *key;
	file_t *file = file_create();

	if (bson_iter_init(&iter, doc)) {
		while (bson_iter_next(&iter)) {
			key = bson_iter_key(&iter);
			value = bson_iter_value(&iter);

			if (strcmp(key, "_id") == 0) {
				strcpy(file->id, value->value.v_utf8.str);
			} else if (strcmp(key, "name") == 0) {
				strcpy(file->name, value->value.v_utf8.str);
			} else if (strcmp(key, "size") == 0) {
				file->size = value->value.v_int32;
			} else if (strcmp(key, "parentId") == 0) {
				strcpy(file->parentId, value->value.v_utf8.str);
			}
		}
	}

	return file;
}

file_t* file_create() {
	file_t* file = malloc(sizeof(file_t));
	file->name = malloc(sizeof(char) * 512);
	file->parentId = malloc(sizeof(char) * 25);
	return file;
}

void file_free(file_t *file) {
	free(file->name);
	free(file->parentId);
	free(file);
}
