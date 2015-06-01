#include "mongo.h"
#include "mongo_file.h"

void mongo_file_checkInit();

mongoc_collection_t *fileCollection;

void mongo_file_checkInit() {
	if (fileCollection == 0) {
		mongo_file_init();
	}
}

bool mongo_file_init() {
	mongoc_client_t *client = mongo_getClient();

	fileCollection = mongoc_client_get_collection(client, "filesystem", "file");

	// Create index to avoid duplicate files in the same path.
	const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1), "parentId", BCON_INT32(1));
	mongo_createIndexIfAbsent(fileCollection, "name_1_parentId_1", indexKeys, 1);
	bson_destroy((bson_t *) indexKeys);

	return EXIT_SUCCESS;
}

void mongo_file_shutdown() {
	mongoc_collection_destroy(fileCollection);
}

bool mongo_file_save(file_t *file) {

	mongo_file_checkInit();

	mongo_generateId(file->id);

	return mongo_saveDoc(file_getBSON(file), fileCollection);
}

file_t* mongo_file_getById(char id[25]) {
	const bson_t *doc;

	mongo_file_checkInit();

	doc = mongo_getDocById(id, fileCollection);

	return file_getFileFromBSON(doc);
}

t_list* mongo_file_getByParentId(char *parentId) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("parentId", BCON_UTF8(parentId));

	return mongo_getByQuery(query, (void*) file_getFileFromBSON, fileCollection);
}

file_t* mongo_file_getByNameInDir(char *name, char *parentId) {
	bson_t *query;
	const bson_t *doc;

	mongo_file_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	doc = mongo_getDocByQuery(query, fileCollection);
	if (doc) {
		return file_getFileFromBSON(doc);
	}

	return NULL;
}

bool mongo_file_deleteFileByNameInDir(char *name, char *parentId) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	return mongo_deleteDocByQuery(query, fileCollection);
}

bool mongo_file_deleteAll() {
	mongo_file_checkInit();

	return mongo_deleteDocByQuery(bson_new(), fileCollection);
}

void mongo_file_updateParentId(char *id, char *newParentId) {
	bson_t *query;
	bson_t *update;

	mongo_file_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));
	update = BCON_NEW("$set", "{", "parentId", BCON_UTF8(newParentId), "}");

	mongo_update(query, update, fileCollection);
}

