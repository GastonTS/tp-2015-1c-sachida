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
	/*
	 const bson_t *indexKeys = BCON_NEW("name", BCON_INT32(1));
	 mongo_createIndexIfAbsent(nodeCollection, "name_1", indexKeys, 1);
	 bson_destroy((bson_t *) indexKeys);
	 */
}

void mongo_node_shutdown() {
	mongoc_collection_destroy(nodeCollection);
}

bool mongo_node_save(node_t *node) {

	mongo_node_checkInit();

	return mongo_saveDoc(nodeCollection, node_getBSON(node));
}

node_t* mongo_node_getById(char *id) {

	mongo_node_checkInit();

	return mongo_getDocById(nodeCollection, id, (void*) node_getNodeFromBSON);
}

t_list* mongo_node_getAll() {

	mongo_node_checkInit();

	return mongo_getByQuery(nodeCollection, bson_new(), (void*) node_getNodeFromBSON);
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
	update = BCON_NEW("$set", "{", "blocks", BCON_BIN(BSON_SUBTYPE_BINARY, (const uint8_t * ) node->blocks->bitarray, node->blocks->size), "blocksCount", BCON_INT32(node->blocksCount), "}");

	mongo_update(nodeCollection, query, update);
}
