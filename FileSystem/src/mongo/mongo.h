#ifndef MONGO_H
#define MONGO_H

#include <commons/collections/list.h>
#include <bcon.h>
#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

#define ID_SIZE sizeof(char) * 25

mongoc_client_t* mongo_getClient();
void mongo_generateId();
void mongo_shutdown();

void mongo_createIndexIfAbsent(mongoc_collection_t *collection, char *name, const bson_t *keys, bool unique);

int mongo_saveDoc(mongoc_collection_t *collection, bson_t *doc);

t_list* mongo_getByQuery(mongoc_collection_t *collection, bson_t *query, void* (*parser)(const bson_t*));
void* mongo_getDocById(mongoc_collection_t *collection, char *id, void* (*parser)(const bson_t*));
void* mongo_getDocByQuery(mongoc_collection_t *collection, bson_t *query, void* (*parser)(const bson_t*));

bool mongo_deleteDocByQuery(mongoc_collection_t *collection, bson_t *query);

void mongo_update(mongoc_collection_t *collection, bson_t *query, bson_t *update);

#endif
