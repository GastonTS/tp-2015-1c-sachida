#ifndef MONGO_FILE_H
#define MONGO_FILE_H

#include "../structs/file.h"

int mongo_file_init();
void mongo_file_shutdown();

int mongo_file_save(file_t *file);

t_list* mongo_file_getByParentId(char *parentId);
file_t* mongo_file_getById(char id[25]);
file_t* mongo_file_getByNameInDir(char *name, char *parentId);

bool mongo_file_deleteFileByNameInDir(char *name, char *parentId);

void mongo_file_updateParentId(char *id, char *newParentId);

#endif
