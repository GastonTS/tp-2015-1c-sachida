#include "mongo.h"
#include "mongo_dir.h"

void mongo_dir_checkInit();

mongoc_collection_t *dirCollection;

void mongo_dir_checkInit() {
	if (dirCollection == 0) {
		mongo_dir_init();
	}
}

bool mongo_dir_init() {
	mongoc_client_t *client = mongo_getClient();

	dirCollection = mongoc_client_get_collection(client, "filesystem", "dir");

	// Create index to avoid duplicate dirs in the same path.
	const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1), "parentId", BCON_INT32(1));
	mongo_createIndexIfAbsent(dirCollection, "name_1_parentId_1", indexKeys, 1);
	bson_destroy((bson_t *) indexKeys);

	return EXIT_SUCCESS;
}

void mongo_dir_shutdown() {
	mongoc_collection_destroy(dirCollection);
}

bool mongo_dir_save(dir_t *dir) {

	mongo_dir_checkInit();

	return mongo_saveDoc(dir_getBSON(dir), dirCollection);
}

dir_t* mongo_dir_getById(char id[25]) {
	const bson_t *doc;

	mongo_dir_checkInit();

	doc = mongo_getDocById(id, dirCollection);

	return dir_getDirFromBSON(doc);
}

t_list* mongo_dir_getByParentId(char *parentId) {
	bson_t *query;

	mongo_dir_checkInit();

	query = BCON_NEW("parentId", BCON_UTF8(parentId));

	return mongo_getByQuery(query, (void*) dir_getDirFromBSON, dirCollection);
}

dir_t* mongo_dir_getByNameInDir(char *name, char *parentId) {
	bson_t *query;
	const bson_t *doc;

	mongo_dir_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	doc = mongo_getDocByQuery(query, dirCollection);
	if (doc) {
		return dir_getDirFromBSON(doc);
	}

	return NULL;
}

bool mongo_dir_deleteDirByNameInDir(char *name, char *parentId) {
	bson_t *query;

	mongo_dir_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	return mongo_deleteDocByQuery(query, dirCollection);

	// TODO, delete files? and folders recursively. do here?
}

bool mongo_dir_deleteAll() {
	mongo_dir_checkInit();

	return mongo_deleteDocByQuery(bson_new(), dirCollection);
}

void mongo_dir_updateParentId(char *id, char *newParentId) {
	bson_t *query;
	bson_t *update;

	mongo_dir_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));
	update = BCON_NEW("$set", "{", "parentId", BCON_UTF8(newParentId), "}");

	mongo_update(query, update, dirCollection);
}
