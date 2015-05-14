#include <bcon.h>
#include <bson.h>
#include <mongoc.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"

bson_t* file_getBSON(file_t *file) {
	bson_oid_t oid;

	bson_oid_init(&oid, NULL);
	bson_oid_to_string(&oid, file->id); // avoid object id.
	return BCON_NEW("_id", BCON_UTF8(file->id), "name", BCON_UTF8(file->name), "size", BCON_INT32(file->size), "parentId", BCON_UTF8(file->parentId));
}
