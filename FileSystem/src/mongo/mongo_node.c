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
	//TODO: index por name ?
	return EXIT_SUCCESS;
}

void mongo_node_shutdown() {
	mongoc_collection_destroy(nodeCollection);
}

bool mongo_node_save(node_t *node) {

	mongo_node_checkInit();

	return mongo_saveDoc(node_getBSON(node), nodeCollection);
}

node_t* mongo_node_getById(char id[25]) {
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
