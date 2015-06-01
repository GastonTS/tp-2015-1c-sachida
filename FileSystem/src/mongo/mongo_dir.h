#ifndef MONGO_DIR_H
#define MONGO_DIR_H

#include "../structs/dir.h"

bool mongo_dir_init();
void mongo_dir_shutdown();

bool mongo_dir_save(dir_t *file);

t_list* mongo_dir_getByParentId(char *parentId);
dir_t* mongo_dir_getById(char id[25]);
dir_t* mongo_dir_getByNameInDir(char *name, char *parentId);

bool mongo_dir_deleteDirByNameInDir(char *name, char *parentId);
bool mongo_dir_deleteById(char *id);
bool mongo_dir_deleteAll();

void mongo_dir_updateParentId(char *id, char *newParentId);

#endif
