#include "mongo.h"
#include "mongo_node.h"

void mongo_node_checkInit();

mongoc_collection_t *nodeCollection;

void mongo_node_checkInit() {
	if (nodeCollection == 0) {
		mongo_node_init();
	}
}

bool mongo_node_init() {
	mongoc_client_t *client = mongo_getClient();

	nodeCollection = mongoc_client_get_collection(client, "filesystem", "node");

	return EXIT_SUCCESS;
}

void mongo_node_shutdown() {
	mongoc_collection_destroy(nodeCollection);
}

bool mongo_node_save(node_t *node) {

	mongo_node_checkInit();

	mongo_generateId(node->id);

	return mongo_saveDoc(node_getBSON(node), nodeCollection);
}

node_t* mongo_node_getById(char id[25]) {
	const bson_t *doc;

	mongo_node_checkInit();

	doc = mongo_getDocById(id, nodeCollection);

	return node_getNodeFromBSON(doc);
}

bool mongo_node_deleteAll() {
	mongo_node_checkInit();

	return mongo_deleteDocByQuery(bson_new(), nodeCollection);
}
