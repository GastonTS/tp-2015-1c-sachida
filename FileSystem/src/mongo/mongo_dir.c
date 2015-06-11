#include "mongo.h"
#include "mongo_dir.h"

void mongo_dir_checkInit();

mongoc_collection_t *dirCollection;

void mongo_dir_checkInit() {
	if (dirCollection == 0) {
		mongo_dir_init();
	}
}

void mongo_dir_init() {
	mongoc_client_t *client = mongo_getClient();

	dirCollection = mongoc_client_get_collection(client, "filesystem", "dir");

	// Create index to avoid duplicate dirs in the same path.
	const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1), "parentId", BCON_INT32(1));
	mongo_createIndexIfAbsent(dirCollection, "name_1_parentId_1", indexKeys, 1);
	bson_destroy((bson_t *) indexKeys);
}

void mongo_dir_shutdown() {
	mongoc_collection_destroy(dirCollection);
}

bool mongo_dir_save(dir_t *dir) {

	mongo_dir_checkInit();

	mongo_generateId(dir->id);

	return mongo_saveDoc(dirCollection, dir_getBSON(dir));
}

dir_t* mongo_dir_getById(char *id) {

	mongo_dir_checkInit();

	return mongo_getDocById(dirCollection, id, (void*) dir_getDirFromBSON);
}

t_list* mongo_dir_getByParentId(char *parentId) {
	bson_t *query;

	mongo_dir_checkInit();

	query = BCON_NEW("parentId", BCON_UTF8(parentId));

	return mongo_getByQuery(dirCollection, query, (void*) dir_getDirFromBSON);
}

dir_t* mongo_dir_getByNameInDir(char *name, char *parentId) {
	bson_t *query;

	mongo_dir_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	return mongo_getDocByQuery(dirCollection, query, (void*) dir_getDirFromBSON);
}

bool mongo_dir_deleteById(char *id) {
	bson_t *query;

	mongo_dir_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));

	return mongo_deleteDocByQuery(dirCollection, query);
}

bool mongo_dir_deleteAll() {
	mongo_dir_checkInit();

	return mongo_deleteDocByQuery(dirCollection, bson_new());
}

void mongo_dir_updateParentId(char *id, char *newParentId) {
	bson_t *query;
	bson_t *update;

	mongo_dir_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));
	update = BCON_NEW("$set", "{", "parentId", BCON_UTF8(newParentId), "}");

	mongo_update(dirCollection, query, update);
}
