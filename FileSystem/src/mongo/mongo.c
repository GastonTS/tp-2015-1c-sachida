#include "mongo.h"

mongoc_client_t *client;

int mongo_init() {
	mongoc_init(); //Initialize driver

	client = mongoc_client_new("mongodb://localhost:27017");

	return EXIT_SUCCESS;
}

mongoc_client_t* mongo_getClient() {
	if (client == 0) {
		mongo_init();
	}
	return client;
}

void mongo_generateId(char id[25]) {
	bson_oid_t oid;

	bson_oid_init(&oid, NULL);
	bson_oid_to_string(&oid, id); // avoid object id.
}

void mongo_shutdown() {
	mongoc_client_destroy(client);
}

int mongo_saveDoc(bson_t *doc, mongoc_collection_t *collection) {
	bson_error_t error;
	bool r;

	r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error);

	if (!r) {
		fprintf(stderr, "%s\n", error.message);
		return EXIT_FAILURE;
	}

	bson_destroy(doc);
	return EXIT_SUCCESS;
}

/*
 * TODO CON LISTAS DE COMMONS..
 bson_t** mongo_getAll() {
 mongoc_cursor_t *cursor;
 const bson_t *items[10];
 char *str;
 int i = 0;

 mongo_checkInit();

 cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, NULL, NULL, NULL);

 while (mongoc_cursor_next(cursor, &items[i]))

 mongoc_cursor_destroy(cursor);

 return items;
 }
 */


const bson_t* mongo_getDocById(char id[25], mongoc_collection_t *collection) {
	mongoc_cursor_t *cursor;
	const bson_t *item;
	bson_t *query;

	// Query
	query = BCON_NEW("_id", BCON_UTF8(id));

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
	mongoc_cursor_next(cursor, &item);

	mongoc_cursor_destroy(cursor);
	bson_destroy(query);

	return item;
}

const bson_t* mongo_getDocByQuery(bson_t *query, mongoc_collection_t *collection) {
	mongoc_cursor_t *cursor;
	const bson_t *item;
	bool r;

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
	r = mongoc_cursor_next(cursor, &item);

	mongoc_cursor_destroy(cursor);
	bson_destroy(query);

	if (r) {
		return item;
	}

	return NULL;
}


bool mongo_deleteDocByQuery(bson_t *query, mongoc_collection_t *collection) {
	bool r;

	r = mongoc_collection_remove(collection, MONGOC_QUERY_NONE, query, NULL, NULL);

	bson_destroy(query);

	return r;
}


