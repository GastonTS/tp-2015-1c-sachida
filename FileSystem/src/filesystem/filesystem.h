#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../mongo/mongo_dir.h"
#include "../mongo/mongo_file.h"
#include "../mongo/mongo_node.h"
#include <commons/log.h>

extern t_log* mdfs_logger;

// TODO cambiar en la entrega !!!!
#define FILESYSTEM_BLOCK_COPIES 3
#define NODE_BLOCK_SIZE 20 * 1024 * 1024

// Holds an operation to be done. ALL POINTERS ARE FREED BY OTHERS.
typedef struct {
	node_t *node;
	off_t blockIndex;
	char *block;
} nodeBlockSendOperation_t;

void filesystem_initialize();
void filesystem_shutdown();

bool filesystem_format();
unsigned long filesystem_getFreeSpaceKiloBytes();

dir_t* filesystem_getDirById(char *id);
dir_t* filesystem_getDirByNameInDir(char *dirName, char *parentId);
file_t* filesystem_getFileByNameInDir(char *fileName, char *parentId);
t_list* filesystem_getDirsInDir(char *parentId);
t_list* filesystem_getFilesInDir(char *parentId);

bool filesystem_deleteDirByNameInDir(char *dirName, char *parentId);
bool filesystem_deleteDir(dir_t *dir);
bool filesystem_deleteFileByNameInDir(char *fileName, char *parentId);
bool filesystem_deleteFile(file_t *file);

void filesystem_moveFile(file_t *file, char *destinationId);
void filesystem_moveDir(dir_t *dir, char *destinationId);
void filesystem_renameFile(file_t *file, char *newName);
void filesystem_renameDir(dir_t *dir, char *newName);

int filesystem_copyFileFromFS(char *route, file_t *file);
bool filesystem_addDir(dir_t *dir);

node_t* filesystem_getNodeById(char *nodeId);
void filesystem_addNode(char *nodeId, uint16_t blocksCount, bool isNewNode);
bool filesystem_deactivateNode(node_t *node);
bool filesystem_activateNode(node_t *node);

char* filesystem_md5sum(file_t* file);

file_t* filesystem_resolveFilePath(char *path, char *startingDirId, char *startingPath);
dir_t* filesystem_resolveDirPath(char *path, char *startingDirId, char *startingPath, char **fullPath);

int filesystem_saveFileBlockToLocalFS(file_t *file, uint16_t blockIndex, char *pathToFile);
int filesystem_makeNewFileBlockCopy(file_t *file, uint16_t blockIndex);
int filesystem_deleteFileBlockCopy(file_t *file, uint16_t blockIndex, uint16_t copyIndex);
int filesystem_saveFileToLocalFS(file_t *file, char *pathToFile);

bool filesystem_copyTmpFileToMDFS(char *nodeId, char *finalTmpName, char *resultFileName);

#endif
