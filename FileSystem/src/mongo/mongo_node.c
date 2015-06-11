#include "mongo.h"
#include "mongo_node.h"

void mongo_node_checkInit();

mongoc_collection_t *nodeCollection;

void mongo_node_checkInit() {
	if (nodeCollection == 0) {
		mongo_node_init();
	}
}

void mongo_node_init() {
	mongoc_client_t *client = mongo_getClient();

	nodeCollection = mongoc_client_get_collection(client, "filesystem", "node");

	// Create index to avoid duplicate nodes.
	const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1));
	mongo_createIndexIfAbsent(nodeCollection, "name_1", indexKeys, 1);
	bson_destroy((bson_t *) indexKeys);
}

void mongo_node_shutdown() {
	mongoc_collection_destroy(nodeCollection);
}

bool mongo_node_save(node_t *node) {

	mongo_node_checkInit();

	mongo_generateId(node->id);

	return mongo_saveDoc(nodeCollection, node_getBSON(node));
}

t_list* mongo_node_getAll() {

	mongo_node_checkInit();

	return mongo_getByQuery(nodeCollection, bson_new(), (void*) node_getNodeFromBSON);
}

node_t* mongo_node_getById(char *id) {

	mongo_node_checkInit();

	return mongo_getDocById(nodeCollection, id, (void*) node_getNodeFromBSON);
}

node_t* mongo_node_getByName(char *name) {
	bson_t *query;

	mongo_node_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name));

	return mongo_getDocByQuery(nodeCollection, query, (void*) node_getNodeFromBSON);
}

bool mongo_node_deleteAll() {
	mongo_node_checkInit();

	return mongo_deleteDocByQuery(nodeCollection, bson_new());
}

void mongo_node_updateBlocks(node_t *node) {
	bson_t *query;
	bson_t *update;

	mongo_node_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(node->id));
	update = BCON_NEW("$set", "{", "blocks", BCON_BIN(BSON_SUBTYPE_BINARY, (const uint8_t * ) node->blocks->bitarray, node->blocks->size), "}");

	mongo_update(nodeCollection, query, update);
}
