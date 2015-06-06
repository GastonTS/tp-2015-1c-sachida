/*
 * Nodo.h
 *
 *  Created on: 1/6/2015
 *      Author: utnso
 */

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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>///hilos

//Variables y tipos de datos
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro
#define SIZE_BLOQUE 20 //Bloques de 20 MB en espacio de datos
#define CANT_BLOQUES 50 //Cantidad de bloques en espacio de datos
#define SIZE_MSG sizeof(t_mensaje)
#define HANDSHAKE 100
#define HANDSHAKEOK 110
#define MARTA 100
#define FILESYSTEM 200
#define JOB 300
#define NODO 400

typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;


int puerto_fs;
int puerto_nodo;
char ip_nodo[16];
char ip_fs[16];
char* archivo_bin;
char* dir_tmp;
char nodo_nuevo[2];
t_log* logger;

//METODOS
void getInfoConf(char* conf);

#endif /* NODO_H_ */


