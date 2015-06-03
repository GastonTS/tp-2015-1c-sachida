#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../mongo/mongo_dir.h"
#include "../mongo/mongo_file.h"
#include "../mongo/mongo_node.h"

bool filesystem_format();

dir_t* filesystem_getDirById(char *id);
dir_t* filesystem_getDirByNameInDir(char *dirName, char *parentId);
file_t* filesystem_getFileByNameInDir(char *fileName, char *parentId);
t_list* filesystem_getDirsInDir(char *parentId);
t_list* filesystem_getFilesInDir(char *parentId);

bool filesystem_deleteDirByNameInDir(char *dirName, char *parentId);
bool filesystem_deleteDirById(char *id);
bool filesystem_deleteFileByNameInDir(char *fileName, char *parentId);
bool filesystem_deleteFileById(char *id);

void filesystem_moveFile(file_t *file, char *destinationId);
void filesystem_moveDir(dir_t *dir, char *destinationId);

bool filesystem_addFile(file_t *file);
bool filesystem_addDir(dir_t *dir);

node_t* filesystem_getNodeByName(char *nodeName);

char* filesystem_md5sum(file_t* file);

#endif