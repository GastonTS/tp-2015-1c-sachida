/*
 * Job.h
 *
 *  Created on: 4/6/2015
 *      Author: utnso
 */

#ifndef JOB_H_
#define JOB_H_

#include <stdlib.h>
#include <stdbool.h> //boleans
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>//socket
#include <arpa/inet.h> //socket
#include <commons/log.h> // log_create, log_info, log_error
#include <commons/config.h> // config_get_int_value
#include <commons/collections/list.h> //agrego listas
#include <commons/bitarray.h>
#include <signal.h>
#include <pthread.h>///hilos

#define BACKLOG 2 /* El número de conexiones permitidas */
#define MAXDATASIZE 100 //Cantidad maxima de datos que puedo mandar de un socket a otro
#define LISTA_ARCHIVOS 100// Para los nombres de los archivos
#define SIZE_MSG sizeof(t_mensaje)
#define HANDSHAKE 100
#define HANDSHAKEOK 110
#define REDUCE 120
#define	MAP 130
#define ARCHIVOS 140
#define RUTINAMAP 150
#define RUTINAREDUCE 160
#define BLOQUE 170
#define MAPOK 180
#define MAPERROR 181
#define REDUCEOK 190
#define REDUCEERROR 191
#define MARTA 200
#define FILESYSTEM 300
#define JOB 400
#define NODO 500

t_log* logger;
int sock_marta;




#endif /* JOB_H_ */
