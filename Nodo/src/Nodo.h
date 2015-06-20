#ifndef NODO_H_
#define NODO_H_

/*Este archivo contiene todas las definiciones, incluciones de librerias, variables,
  metodos, y estructuras que vamos a utilizar en el Nodo.c
*/

//Librerias
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> //es para los fd_set
#include <errno.h>
#include <commons/config.h> // para el archivo de config
#include <commons/log.h> // log_create, log_info, log_error
#include <commons/string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>///hilos
#include "../../utils/socket.h"
#include <string.h>

//Variables y tipos de datos
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro
#define CANT_BLOQUES 50 //Cantidad de bloques en espacio de datos
#define SIZE_MSG sizeof(t_mensaje)
#define HANDSHAKE 100
#define HANDSHAKEOK 110
#define MARTA 100
#define FILESYSTEM 200
#define JOB 300
#define NODO 400


#define	BLOCK_SIZE	20 * 1024 * 1024 // 20 MB de bloques

typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;

t_log* nodeLogger;

//METODOS
int initConfig(char* conf);

char* getBloque(uint16_t nroBloque);

void setBloque(uint16_t nroBloque, char* string);

#endif /* NODO_H_ */


