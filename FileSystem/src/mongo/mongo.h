#ifndef MONGO_H
#define MONGO_H

#include <bcon.h>
#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>

mongoc_client_t* mongo_getClient();
void mongo_generateId();
void mongo_shutdown();
int mongo_saveDoc(bson_t *doc, mongoc_collection_t *collection);
const bson_t* mongo_getDocById(char id[25], mongoc_collection_t *collection);
const bson_t* mongo_getDocByQuery(bson_t *query, mongoc_collection_t *collection);
bool mongo_deleteDocByQuery(bson_t *query, mongoc_collection_t *collection);

//bson_t** mongo_getAll();

#endif
