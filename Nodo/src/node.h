#ifndef NODE_H
#define NODE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include "utils/socket.h"
#include <semaphore.h>

#define BLOCK_SIZE 20 * 1024 * 1024 // 20 MB de bloques

typedef struct {
	char *fsIp;
	uint16_t fsPort;
	uint16_t listenPort;
	char *binFilePath;
	char *tmpDir;
	bool newNode;
	char *name;
	uint16_t blocksCount;
} t_nodeCfg;

extern sem_t routines_sem;
extern t_log* node_logger;
extern t_nodeCfg* node_config;

char* node_getBlock(uint16_t numBlock);
void node_setBlock(uint16_t numBlock, char *blockData);
e_socket_status node_setBlockFromPacket(uint16_t numBlock, int socket);

bool node_executeMapRutine(char *mapRutine, uint16_t numBlock, char *tmpFileName);
bool node_executeReduceRutine(char *reduceRutine, char *tmpFileNameToReduce, char *finalTmpFileName);
char* node_getTmpFileContent(char *tmpFileName);
bool node_createTmpFileFromStringList(char *pathToFile, t_list *stringParts);

#endif
