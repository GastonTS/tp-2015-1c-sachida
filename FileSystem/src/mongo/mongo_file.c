#include "mongo.h"
#include "mongo_file.h"

void mongo_file_checkInit();

mongoc_collection_t *fileCollection;

void mongo_file_checkInit() {
	if (fileCollection == 0) {
		mongo_file_init();
	}
}

void mongo_file_init() {
	mongoc_client_t *client = mongo_getClient();

	fileCollection = mongoc_client_get_collection(client, "filesystem", "file");

	// Create index to avoid duplicate files in the same path.
	const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1), "parentId", BCON_INT32(1));
	mongo_createIndexIfAbsent(fileCollection, "name_1_parentId_1", indexKeys, 1);
	bson_destroy((bson_t *) indexKeys);
}

void mongo_file_shutdown() {
	mongoc_collection_destroy(fileCollection);
}

bool mongo_file_save(file_t *file) {

	mongo_file_checkInit();

	mongo_generateId(file->id);

	return mongo_saveDoc(fileCollection, file_getBSON(file));
}

file_t* mongo_file_getById(char *id) {

	mongo_file_checkInit();

	return mongo_getDocById(fileCollection, id, (void*) file_getFileFromBSON);
}

t_list* mongo_file_getByParentId(char *parentId) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("parentId", BCON_UTF8(parentId));

	return mongo_getByQuery(fileCollection, query, (void*) file_getFileFromBSON);
}

t_list* mongo_file_getFilesThatHaveNode(char *nodeId) {
	bson_t *query;

	mongo_file_checkInit();

	//db.file.find({blocks: {$elemMatch: { $elemMatch: { nodeId: "55776dea508b964de617a22" }   }   }})
	query = BCON_NEW("blocks", "{", "$elemMatch", "{", "$elemMatch", "{", "nodeId", BCON_UTF8(nodeId), "}", "}", "}");

	return mongo_getByQuery(fileCollection, query, (void*) file_getFileFromBSON);
}

file_t* mongo_file_getByNameInDir(char *name, char *parentId) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	return mongo_getDocByQuery(fileCollection, query, (void*) file_getFileFromBSON);
}

bool mongo_file_deleteById(char *id) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));

	return mongo_deleteDocByQuery(fileCollection, query);
}

bool mongo_file_deleteAll() {
	mongo_file_checkInit();

	return mongo_deleteDocByQuery(fileCollection, bson_new());
}

void mongo_file_updateParentId(char *id, char *newParentId) {
	bson_t *query;
	bson_t *update;

	mongo_file_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));
	update = BCON_NEW("$set", "{", "parentId", BCON_UTF8(newParentId), "}");

	mongo_update(fileCollection, query, update);
}

void mongo_file_addBlockCopyToFile(char *id, uint16_t blockIndex, file_block_t *fileBlockCopy) {
	bson_t *query;
	bson_t *update;

	mongo_file_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));

	char blockKey[128];
	snprintf(blockKey, sizeof(blockKey), "blocks.%d", blockIndex);

	update = BCON_NEW("$addToSet", "{", blockKey, BCON_DOCUMENT(file_block_getBSON(fileBlockCopy)), "}");

	mongo_update(fileCollection, query, update);
}

void mongo_file_deleteBlockCopy(char *id, uint16_t blockIndex, file_block_t *fileBlockCopy) {
	bson_t *query;
	bson_t *update;

	mongo_file_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(id));

	char blockKey[128];
	snprintf(blockKey, sizeof(blockKey), "blocks.%d", blockIndex);

	update = BCON_NEW("$pull", "{", blockKey, BCON_DOCUMENT(file_block_getBSON(fileBlockCopy)), "}");

	mongo_update(fileCollection, query, update);
}

