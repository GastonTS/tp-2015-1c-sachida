#include <bcon.h>
#include <bson.h>
#include <mongoc.h>
#include <stdio.h>
#include <stdlib.h>
#include "../structs/file.h"

void mongo_checkInit();
int mongo_init();
void mongo_shutdown();
void mongo_getFile(char id[25]);
int mongo_saveFile(file_t *file);

