/*
 * servidor.h
 *
 *  Created on: 12/5/2015
 *      Author: utnso
 */
//EN ESTE ARCHIVO HAGO TODAS LAS DEFINICIONES DE VARIABLES Y FUNCIONES QUE NECESITE UTILIZAR
//PARA QUE SEA MAS LEGIBLE EN EL .C

#ifndef SERVIDOR_H_
#define SERVIDOR_H_

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
#include <commons/collections/list.h>

//Variables y tipos de datos
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro
#define SIZE_MSG sizeof(t_mensaje)
#define HANDSHAKE 100
#define HANDSHAKEOK 110
#define REDUCE 120
#define	MAP 130
#define ARCHIVOS 140
#define MARTA 200
#define FILESYSTEM 300
#define JOB 400
#define NODO 500

typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	t_list* list_archivos;
	char mensaje[16];
} t_mensaje;

t_log* logger;
t_list* list_archivos;
int puerto_servidor;
char ip_servidor[16];


//Metodos
int escuchar();
int escucharPuerto(int sockEscucha);
void getInfoConf(char* archivo_config);




#endif /* SERVIDOR_H_ */
