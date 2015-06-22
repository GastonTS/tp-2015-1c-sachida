#ifndef NODO_H_
#define NODO_H_

//Librerias
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/string.h>

#define	BLOCK_SIZE	20 * 1024 * 1024 // 20 MB de bloques

typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;

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

extern t_log* node_logger;
extern t_nodeCfg* node_config;

char* node_getBlock(uint16_t numBlock);
void node_freeBlock(char *blockStr);
void node_setBlock(uint16_t numBlock, char *blockData);

#endif /* NODO_H_ */

