#include "mongo.h"

mongoc_collection_t *collection;
mongoc_client_t *client;

void mongo_checkInit() {
	if (collection == 0 || client == 0) {
		mongo_init();
	}
}

int mongo_init() {
	mongoc_init(); //Initialize driver

	client = mongoc_client_new("mongodb://localhost:27017");
	collection = mongoc_client_get_collection(client, "filesystem", "file");

	return EXIT_SUCCESS;
}

void mongo_shutdown() {
	mongoc_collection_destroy(collection);
	mongoc_client_destroy(client);
}

void mongo_getFile(char id[25]) {
	mongoc_cursor_t *cursor;
	const bson_t *item;
	bson_t *query;
	char *str;

	mongo_checkInit();

	// Query
	query = BCON_NEW("_id", BCON_UTF8(id));

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);
	while (mongoc_cursor_next(cursor, &item)) {
		str = bson_as_json(item, NULL);
		printf("%s\n", str);
		bson_free(str);
	}

	mongoc_cursor_destroy(cursor);
	bson_destroy(query);
}

int mongo_saveFile(file_t *file) {
	bson_error_t error;
	bson_t *doc;
	bool r;

	mongo_checkInit();

	doc = file_getBSON(file);
	r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error);

	if (!r) {
		fprintf(stderr, "%s\n", error.message);
		return EXIT_FAILURE;
	}

	bson_destroy(doc);
	return EXIT_SUCCESS;
}


