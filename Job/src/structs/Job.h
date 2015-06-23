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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "node.h"

typedef struct {
	uint16_t PUERTO_MARTA;
	char* IP_MARTA;
	char* MAPPER;
	char* REDUCER;
	char* RESULTADO;
	char* LIST_ARCHIVOS;
	char*  COMBINER;
} t_configJob;


struct parms_threads{
	void *buffer;
	size_t tamanio;
};


t_log* logger;
t_configJob* cfgJob;
pthread_t hilo_mapper;
pthread_t hilo_reduce;
int sock_marta;


/**************** METODOS ***************/
/* Estructuras */
int initConfig(char* configFile);
void freeCfg();
void freeThreadMap(t_map* map);
void freeThreadReduce(t_reduce* reduce);



#endif /* JOB_H_ */
