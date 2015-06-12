#ifndef MONGO_FILE_H
#define MONGO_FILE_H

#include "../structs/file.h"

void mongo_file_init();
void mongo_file_shutdown();

bool mongo_file_save(file_t *file);

file_t* mongo_file_getById(char *id);
t_list* mongo_file_getByParentId(char *parentId);
t_list* mongo_file_getFilesThatHaveNode(char *nodeId);
file_t* mongo_file_getByNameInDir(char *name, char *parentId);

bool mongo_file_deleteById(char *id);
bool mongo_file_deleteAll();

void mongo_file_updateParentId(char *id, char *newParentId);

#endif
