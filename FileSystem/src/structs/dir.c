#include <bcon.h>
#include <bson.h>
#include <stdlib.h>
#include <string.h>

#include "../mongo/mongo.h"
#include "dir.h"

bson_t* dir_getBSON(dir_t *dir) {

	mongo_generateId(dir->id);
	return BCON_NEW(
			"_id", BCON_UTF8(dir->id),
			"name", BCON_UTF8(dir->name),
			"parentId", BCON_UTF8(dir->parentId)
		);
}


dir_t* dir_getDirFromBSON(const bson_t *doc) {
	bson_iter_t iter;
	const bson_value_t *value = malloc(sizeof(bson_value_t *));
	char *key = malloc(sizeof(char*));
	dir_t *dir = malloc(sizeof(dir_t));

	if (bson_iter_init (&iter, doc)) {
	   while (bson_iter_next(&iter)) {
		   strcpy(key, bson_iter_key(&iter));
		   value = bson_iter_value(&iter);

		   if (strcmp(key, "_id") == 0) {
			   strcpy(dir->id, value->value.v_utf8.str);
		   } else if (strcmp(key, "name") == 0) {
			   strcpy(dir->name, value->value.v_utf8.str);
		   } else if (strcmp(key, "parentId") == 0) {
			   strcpy(dir->parentId, value->value.v_utf8.str);
		   }
	   }
	}

	return dir;
}
