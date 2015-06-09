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

	return mongo_saveDoc(node_getBSON(node), nodeCollection);
}

t_list* mongo_node_getAll() {

	mongo_node_checkInit();

	return mongo_getByQuery(bson_new(), (void*) node_getNodeFromBSON, nodeCollection);
}

node_t* mongo_node_getById(char id[]) {
	const bson_t *doc;

	mongo_node_checkInit();

	doc = mongo_getDocById(id, nodeCollection);

	return node_getNodeFromBSON(doc);
}

node_t* mongo_node_getByName(char *name) {
	bson_t *query;
	const bson_t *doc;

	mongo_node_checkInit();

	query = BCON_NEW("name", BCON_UTF8(name));

	doc = mongo_getDocByQuery(query, nodeCollection);

	if (doc) {
		return node_getNodeFromBSON(doc);
	}

	return NULL;

}

bool mongo_node_deleteAll() {
	mongo_node_checkInit();

	return mongo_deleteDocByQuery(bson_new(), nodeCollection);
}

void mongo_node_updateBlocks(node_t *node) {
	bson_t *query;
	bson_t *update;

	mongo_node_checkInit();

	query = BCON_NEW("_id", BCON_UTF8(node->id));
	update = BCON_NEW("$set", "{", "blocks", BCON_BIN(BSON_SUBTYPE_BINARY, (const uint8_t * ) node->blocks->bitarray, node->blocks->size), "}");

	mongo_update(query, update, nodeCollection);
}
