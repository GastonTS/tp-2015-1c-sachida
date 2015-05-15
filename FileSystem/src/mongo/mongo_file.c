#include "mongo.h"
#include "mongo_file.h"

mongoc_collection_t *fileCollection;

void mongo_file_checkInit() {
	if (fileCollection == 0) {
		mongo_file_init();
	}
}

int mongo_file_init() {
	// TODO, ensure indexes (name|parentId)
	mongoc_client_t *client = mongo_getClient();

	fileCollection = mongoc_client_get_collection(client, "filesystem", "file");

	return EXIT_SUCCESS;
}

void mongo_file_shutdown() {
	mongoc_collection_destroy(fileCollection);
}

int mongo_file_save(file_t *file) {
	bool r;

	mongo_file_checkInit();

	r = mongo_saveDoc(file_getBSON(file), fileCollection);

	if (!r) {
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

file_t* mongo_file_getById(char id[25]) {
	const bson_t *doc;

	mongo_file_checkInit();

	doc = mongo_getDocById(id, fileCollection);

	return file_getFileFromBSON(doc);
}

bool mongo_file_deleteFileByNameInDir(char *name, char parentId[25]) {
	bson_t *query;

	mongo_file_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name), "parentId", BCON_UTF8(parentId));

	return mongo_deleteDocByQuery(query, fileCollection);
}

