#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utils/socket.h"
#include <commons/log.h> // log_create, log_info, log_error

void startFS();
void nodo_escucharAcciones(int fsSocket);

void deserialzeSetBloque(void *buffer);
void setBloque(uint16_t numBlock, char *blockData);

void deserialzeGetBloque(void *buffer, int fsSocket);
char* getBloque(uint16_t numBlock);

t_log* logger;
