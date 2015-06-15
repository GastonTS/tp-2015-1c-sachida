#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utils/socket.h"

void startMarta();
void marta_escucharAcciones(int fsSocket);
void pedirBloquesArchivo(int fsSocket);

typedef struct {
	char *nodeId;
	uint16_t blockIndex;
} file_block_t;
