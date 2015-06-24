/*
 * marta.c
 *
 *  Created on: 23/6/2015
 *      Author: utnso
 */
#include "../structs/Job.h"
#include "../utils/socket.h"
#include "../structs/node.h"
#include "marta.h"

//**********************************************************************************//
//									CONEXIONES MaRTA								//
//**********************************************************************************//

int conectarMarta() {
	int handshake;

	if ((sock_marta = socket_connect(cfgJob->IP_MARTA, cfgJob->PUERTO_MARTA)) < 0) {
		log_error(logger, "Error al conectar con MaRTA %d", sock_marta);
		freeCfg();
		return EXIT_FAILURE;
	}

	log_info(logger, "Coneccion con MaRTA: %d", sock_marta);

	handshake = socket_handshake_to_server(sock_marta, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	if (!handshake) {
		log_error(logger,"Error en handshake con MaRTA");
		freeCfg();
		socket_close(sock_marta);
		return EXIT_FAILURE;
	}

	return sock_marta;
}


void atenderMarta(int socketMarta) {

	/* Mando el Config a MaRTA */
	serializeConfigMaRTA(sock_marta, cfgJob->COMBINER, cfgJob->LIST_ARCHIVOS);
	log_info(logger,"SerializeConfigMaRTA OK");
	while (1) {
		/*Recibo Orden de Map o Reduce */
		recvOrder(sock_marta);
	}
}

//**********************************************************************************//
//									PAQUETES MaRTA									//
//**********************************************************************************//

/* MANDO MIS CONFIGURACIONES INICIALES A MaRTA */
void serializeConfigMaRTA(int fd, bool combiner, char* stringFiles) {

	size_t filesLength = sizeof(stringFiles);
	size_t scombiner = sizeof(combiner);
	size_t sbuffer = scombiner + filesLength;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	combiner = htons(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy((buffer + scombiner), stringFiles, filesLength);
	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}

/* RECIBO ORDEN DE MAP O REDUCE DE MaRTA */
void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	char order = '\0';
	size_t sOrder = sizeof(char);
	memcpy(&order, buffer, sOrder);
	log_info(logger,"recOrder");
	/* Map */
	struct parms_threads parms_map;
	parms_map.buffer = (buffer + sOrder);
	parms_map.tamanio = sOrder;

	/* Reduce */
	struct parms_threads parms_reduce;
	parms_reduce.buffer = (buffer + sOrder);
	parms_reduce.tamanio = sbuffer - sOrder;

	printf("\nRECVORDER: %c\n", order);
	if (order == 'm'){
		log_info(logger,"Map Recived");
		pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, &parms_map);
		/*
		hiloMap = malloc(sizeof(t_list_thread));
		contMap++;
		hiloMap->IdThread = contMap;
		hiloMap->Thread = hilo_mapper;
		list_add(list_mappers,hiloMap);
		*/
	}
	else if (order == 'r'){
		log_info(logger,"Reduce Recived");
		pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, &parms_reduce);
		/*
		hiloRed = malloc(sizeof(t_list_thread));
		contReduce++;
		hiloRed->IdThread = contReduce;
		hiloRed->Thread = hilo_reduce
		list_add(list_reducers,hiloRed);
		*/
	}
	else if (order == 'd') {
		printf("\nDIE JOB\n");
		free(buffer);
		freeCfg();
		exit(-1);
	}
}



void atenderMapper(void* parametros) {
	struct parms_threads *p = (struct parms_threads *)parametros;
	log_info(logger, "Thread map created");
	printf("Thread map created");

	t_map* map;

	/* Desserializo el mensaje de Mapper de MaRTA */

	map = desserializeMapOrder(p->buffer);


	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;
	int ret_val = 0;

	if ((sock_nodo = socket_connect(map->ip_nodo, map->port_nodo)) < 0) {
		log_error(logger, "Error al conectar map con Nodo %d", sock_nodo);
		freeThreadMap(map);
		pthread_exit(&ret_val);
	}

	log_info(logger, "Coneccion con Nodo: %d", sock_nodo);

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO, HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger,"Error en map hand_nodo con Nodo %d", hand_nodo);
		freeThreadMap(map);
		pthread_exit(&ret_val);
	}

	/* Serializo y mando datos */
	log_info(logger,"Handshake Map Nodo: %d",hand_nodo);
	serializeMap(sock_nodo,map);

	/* Wait for confirmation */
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(sock_nodo, &buffer, &sbuffer);
	char conf = '\0';
	size_t sConf = sizeof(char);
	memcpy(&conf, buffer, sConf);

	//Desserializar
	confirmarMap(conf, map);
	free(buffer);

}

void confirmarMap(char confirmacion, t_map* map) {

	/*TODO ARMAR METODO? */
	size_t sOrder = sizeof(char);
	size_t sIdJob = sizeof(uint16_t);
	size_t sbuffer = sOrder + sIdJob;

	uint16_t idJob = htons(map->idJob);

	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &confirmacion, sOrder);
	memcpy(buffer + sOrder, &idJob, sIdJob);


	if (confirmacion == 't'){
			socket_send_packet(sock_marta,&buffer,sbuffer);
			log_info(logger,"Map %d Successfully Completed", map->idJob);
			printf("Map %d Successfully Completed \n",map->idJob);
		}
		else if (confirmacion == 'f'){
			socket_send_packet(sock_marta,&buffer,sbuffer);
			log_info(logger,"Map %d Failed", map->idJob);
			printf("Map %d Failed \n",map->idJob);
		}
		else{
			printf("\n NO SE RECONOCIO LA CONFIRMACION DEL NODO \n");
			log_error(logger,"NO SE RECONOCIO LA CONFIRMACION DEL NODO");
		}
	freeThreadMap(map);
	free(buffer);
}

void atenderReducer(void* parametros) {
	struct parms_threads *p = (struct parms_threads *)parametros;
	log_info(logger, "Thread reduce created");
	printf("Thread reduce created");

	t_reduce* reduce;

	/* Desserializo el mensaje de Mapper de MaRTA */
	reduce = desserializeReduceOrder(p->buffer,p->tamanio);

	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;
	int ret_val = 0;

	if ((sock_nodo = socket_connect(reduce->ip_nodo, reduce->port_nodo)) < 0) {
		log_error(logger, "Error al conectar reduce con Nodo %d", sock_nodo);
		freeThreadReduce(reduce);
		pthread_exit(&ret_val);
	}

	log_info(logger, "Coneccion reduce con Nodo: %d", sock_nodo);

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO, HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger,"Error en reduce hand_nodo con Nodo %d", hand_nodo);
		freeThreadReduce(reduce);
		pthread_exit(&ret_val);
	}

	/* Serializo y mando datos */
	log_info(logger,"Handshake Reduce Nodo: %d",hand_nodo);
	serializeReduce(sock_nodo,reduce);


	/* Confirmar Reduce */

	freeThreadReduce(reduce);
}

void confirmarReduce() {

}

//**********************************************************************************//
//									PAQUETES MaRTA 									//
//**********************************************************************************//

//***********************************MAP********************************************//
t_map* desserializeMapOrder(void *buffer) {
	size_t sIdMap = sizeof(uint16_t);
	size_t snumblock = sIdMap;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;
	uint16_t idMap;
	char* nodeIP;
	uint16_t nodePort;
	uint16_t numBlock;
	char tempResultName[60];
	t_map* map;
	map = malloc(sizeof(t_map));

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, stempName);

	//TODO GUARDAR EN ESTRUCTURAS
	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	map = malloc(sizeof(t_map));
	map->idJob = idMap;
	map->ip_nodo = strdup(nodeIP);
	map->port_nodo = nodePort;
	map->numBlock = numBlock;
	map->tempResultName = strdup(tempResultName);


	printf("\nID: %d\n", map->idJob);
	printf("IP:  %s  \n", map->ip_nodo);
	printf("PUERTO: %d\n", map->port_nodo);
	printf("NUMBLOCK: %d\n", map->numBlock);
	printf("%s\n", map->tempResultName);
	fflush(stdout);
	free(nodeIP);
	return(map);

}

//*********************************SerializeConfMap*******************************************//

//*********************************REDUCE*******************************************//
typedef struct {
	uint16_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char tempName[60];
} t_temp;

void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer) {
	t_temp *temporal = malloc(sizeof(t_temp));
	size_t snodeIP;

	memcpy(&temporal->originMap, buffer + *sbuffer, sizeof(uint16_t));
	temporal->originMap = ntohs(temporal->originMap);
	memcpy(&snodeIP, buffer + *sbuffer + sizeof(uint16_t), sizeof(snodeIP));
	temporal->nodeIP = malloc(snodeIP);
	memcpy(temporal->nodeIP, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP), snodeIP);
	memcpy(&temporal->nodePort, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP, sizeof(uint16_t));
	temporal->nodePort = ntohs(temporal->nodePort);
	memcpy(temporal->tempName, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), sizeof(char) * 60);

	list_add(temporals, temporal);
	*sbuffer += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

t_reduce* desserializeReduceOrder(void *buffer, size_t sbuffer) {
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;

	char* nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
	uint16_t countTemps = 0;

	memcpy(&snodeIP, buffer, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sizeof(snodeIP), snodeIP);
	memcpy(&nodePort, buffer + sizeof(snodeIP) + snodeIP, snodePort);
	nodePort = ntohs(nodePort);
	memcpy(tempResultName, buffer + sizeof(snodeIP) + snodeIP + snodePort, stempName);
	memcpy(&countTemps, buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName, sizeof(uint16_t));
	countTemps = ntohs(countTemps);
	void *tempsBuffer = malloc(sbuffer - sizeof(snodeIP) - snodeIP - snodePort - stempName - sizeof(uint16_t));
	tempsBuffer = buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t);
	size_t stempsBuffer = 0;
	t_list *temps = list_create();
	for (; countTemps; countTemps--) {
		desserializeTempToList(temps, tempsBuffer, &stempsBuffer);
	}

	t_reduce* reduce;
	reduce = malloc(sizeof(t_reduce));
	reduce->ip_nodo = strdup(nodeIP);
	reduce->port_nodo = nodePort;
	reduce->tempResultName = strdup(tempResultName);
	//TODO VER COMO GUARDAR LA LISTA
	//reduce->temps;

//Test TODO: Adaptar a las estructuras de Job
	printf("\n%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%s\n", tempResultName);
	printf("Count Temps:%d\n", list_size(temps));
	void showTemp(t_temp *temp) {
		printf("-Temp-\n");
		printf("\t%d\n", temp->originMap);
		printf("\t%s\n", temp->nodeIP);
		printf("\t%d\n", temp->nodePort);
		printf("\t%s\n", temp->tempName);
	}
	list_iterate(temps, (void *) showTemp);

	return (reduce);
//End
}
