/*
 * cliente.h
 *
 *  Created on: 12/5/2015
 *      Author: utnso
 */

#ifndef CLIENTE_H_
#define CLIENTE_H_

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

//Variables y tipos de datos
#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro
#define SIZE_MSG sizeof(t_mensaje)
#define HANDSHAKE 100
#define HANDSHAKEOK 110
#define CLIENTE 200
#define SERVIDOR 300


typedef struct {
	int tipo;
	int id_proceso;
	int datosNumericos;
	char mensaje[16];
} t_mensaje;


int puerto_servidor;
char ip_servidor[16];
t_log* logger;

#endif /* CLIENTE_H_ */
