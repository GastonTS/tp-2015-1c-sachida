#include "mongo.h"

mongoc_client_t *client;

int mongo_init();

int mongo_init() {
	mongoc_init(); //Initialize driver

	client = mongoc_client_new("mongodb://localhost:27017");

	return EXIT_SUCCESS;
}

mongoc_client_t* mongo_getClient() {
	if (!client) {
		mongo_init();
	}
	return client;
}

void mongo_generateId(char *id) {
	bson_oid_t oid;

	bson_oid_init(&oid, NULL);
	bson_oid_to_string(&oid, id); // avoid object id.
}

void mongo_shutdown() {
	mongoc_client_destroy(client);
	mongoc_cleanup();
}

void mongo_createIndexIfAbsent(mongoc_collection_t *collection, char *name, const bson_t *keys, bool unique) {

	mongoc_index_opt_t *opt = malloc(sizeof(mongoc_index_opt_t));
	mongoc_index_opt_init(opt);

	opt->unique = unique;
	opt->background = 1;
	opt->name = name;

	// Creates only if not present.
	mongoc_collection_create_index(collection, keys, opt, NULL);

	free(opt);
}

bool mongo_saveDoc(mongoc_collection_t *collection, bson_t *doc) {
	bson_error_t error;
	bool r;

	r = mongoc_collection_insert(collection, MONGOC_INSERT_NONE, doc, NULL, &error);

	if (!r) {
		fprintf(stderr, "%s\n", error.message);
	}

	bson_destroy(doc);

	return r;
}

t_list* mongo_getByQuery(mongoc_collection_t *collection, bson_t *query, void* (*parser)(const bson_t*)) {
	mongoc_cursor_t *cursor;
	const bson_t *item;
	t_list *items;

	items = list_create();

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 0, 0, query, NULL, NULL);

	while (mongoc_cursor_next(cursor, &item)) {
		list_add(items, parser(item));
	}

	bson_destroy(query);
	mongoc_cursor_destroy(cursor);

	return items;
}

void* mongo_getDocById(mongoc_collection_t *collection, char *id, void* (*parser)(const bson_t*)) {
	bson_t *query;

	query = BCON_NEW("_id", BCON_UTF8(id));

	return mongo_getDocByQuery(collection, query, parser);
}

void* mongo_getDocByQuery(mongoc_collection_t *collection, bson_t *query, void* (*parser)(const bson_t*)) {
	mongoc_cursor_t *cursor;
	const bson_t *item;
	bool r;

	cursor = mongoc_collection_find(collection, MONGOC_QUERY_NONE, 0, 1, 0, query, NULL, NULL);
	r = mongoc_cursor_next(cursor, &item);

	void *parsedItem = NULL;
	if (r) {
		parsedItem = parser(item);
	}

	bson_destroy(query);
	mongoc_cursor_destroy(cursor);

	return parsedItem;
}

bool mongo_deleteDocByQuery(mongoc_collection_t *collection, bson_t *query) {
	bool r;

	r = mongoc_collection_remove(collection, MONGOC_QUERY_NONE, query, NULL, NULL);

	bson_destroy(query);

	return r;
}

void mongo_update(mongoc_collection_t *collection, bson_t *query, bson_t *update) {

	mongoc_collection_update(collection, MONGOC_UPDATE_MULTI_UPDATE, query, update, NULL, NULL);

	bson_destroy(query);
	bson_destroy(update);
}
