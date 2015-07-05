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
		return -1;
	}

	log_info(logger, "Coneccion con MaRTA: %d", sock_marta);

	handshake = socket_handshake_to_server(sock_marta, HANDSHAKE_MARTA,
	HANDSHAKE_JOB);
	if (!handshake) {
		log_error(logger, "Error en handshake con MaRTA");
		freeCfg();
		socket_close(sock_marta);
		return -1;
	}

	return sock_marta;
}

void atenderMarta(int socketMarta) {

	/* Mando el Config a MaRTA */
	serializeConfigMaRTA(sock_marta, cfgJob->COMBINER, cfgJob->RESULTADO, cfgJob->LIST_ARCHIVOS);
	log_info(logger, "SerializeConfigMaRTA OK");
	while (1) {
		/*Recibo Orden de Map o Reduce */
		recvOrder(sock_marta);
	}
}

//**********************************************************************************//
//									PAQUETES MaRTA									//
//**********************************************************************************//

/* MANDO MIS CONFIGURACIONES INICIALES A MaRTA */
void serializeConfigMaRTA(int fd, bool combiner,char* fileResult, char* stringFiles) {
	size_t filesLength = strlen(stringFiles);
	size_t scombiner = sizeof(combiner);
	uint16_t sizeResult = strlen(fileResult);

	/* Htons */
	uint16_t sizeResultSerialize = htons(sizeResult);

	size_t sbuffer = scombiner + sizeof(uint16_t) + sizeResult + filesLength;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	combiner = htons(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy(buffer + scombiner, &sizeResultSerialize, sizeof(uint16_t));
	memcpy(buffer + scombiner + sizeof(uint16_t), fileResult, sizeResult);
	memcpy(buffer + scombiner + sizeof(uint16_t) + sizeResult, stringFiles, filesLength);
	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}

/* RECIBO ORDEN DE MAP O REDUCE DE MaRTA */
void desserializeDieOrder(void *buffer) {
	uint8_t finalResult;
	memcpy(&finalResult, buffer, sizeof(uint8_t));
	if (finalResult == COMMAND_RESULT_OK)
		log_info(logger, "Job Success!");
	else if (finalResult == COMMAND_RESULT_FILEUNAVAILABLE)
		log_info(logger, "Job Failed: One of the files is unavailable");
	else if (finalResult == COMMAND_RESULT_REDUCEFAILED)
		log_info(logger, "Job Failed: Reduce failed");
	else
		log_info(logger, "Job Failed: don't know why");
}

void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	e_socket_status status = 0;
	if ((status = socket_recv_packet(fd, &buffer, &sbuffer)) < 0) {
		log_error(logger, "ABORT --> MaRTA Disconnect");
		socket_close(sock_marta);
		freeCfg(cfgJob);
		exit(-1);
	}

	uint8_t order;
	size_t sOrder = sizeof(uint8_t);
	memcpy(&order, buffer, sOrder);

	printf("\n\n%d\n\n", order);
	fflush(stdout);
	if (order == COMMAND_MAP) {
		log_info(logger, "Map Recived");
		t_map *map = malloc(sizeof(t_map));
		map = desserializeMapOrder(buffer + sOrder);
		pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, (void *) map);
		pthread_detach(hilo_mapper);

	} else if (order == COMMAND_REDUCE) {
		log_info(logger, "Reduce Recived");
		t_reduce *reduce = malloc(sizeof(t_reduce));
		reduce = desserializeReduceOrder(buffer + sOrder, sbuffer - sOrder);
		pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, (void *) reduce);
		pthread_detach(hilo_reduce);
	}

	else if (order == COMMAND_MARTA_TO_JOB_DIE){
		desserializeDieOrder(buffer + sOrder);
		freeCfg();
		free(buffer);
		exit(0);
	}

	free(buffer);
}

void atenderMapper(void * parametros) {
	log_info(logger, "Thread map created");
	t_map *map = (t_map *) parametros;

	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;

	if ((sock_nodo = socket_connect(map->ip_nodo, map->port_nodo)) < 0) {
		log_error(logger, "Error al conectar map con Nodo %d", sock_nodo);
		failMap(map);
		freeThreadMap(map);
		pthread_exit(&sock_nodo);
		return;
	}

	log_info(logger, "Coneccion con Nodo: %d", sock_nodo);

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO,
	HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger, "Error en map hand_nodo con Nodo %d", hand_nodo);
		freeThreadMap(map);
		pthread_exit(&hand_nodo);
		return;
	}

	/* Serializo y mando datos */
	log_info(logger, "Handshake Map Nodo: %d", hand_nodo);
	serializeMap(sock_nodo, map);

	/* Wait for confirmation */

	void *buffer;
	size_t sbuffer;
	e_socket_status status = socket_recv_packet(sock_nodo, &buffer, &sbuffer);

	/* Si se cae el nodo le mando que murio */
	if (status < 0) {
		failMap(map);
		//free(buffer);
	} else {
		bool conf;
		size_t sConf = sizeof(bool);
		memcpy(&conf, buffer, sConf);

		/* Confirmar Map */
		confirmarMap(conf, map);
	}
	free(buffer);
	socket_close(sock_nodo);
	freeThreadMap(map);
	pthread_exit(&status);

}

void failMap(t_map* map){
	uint8_t comando = COMMAND_MAP;
	size_t scomando = sizeof(uint8_t);
	size_t sbool = sizeof(bool);
	bool fallo = 0;
	size_t sidmap = sizeof(uint16_t);
	size_t sbufferConf = scomando + sbool + sidmap;
	void* bufferConf = malloc(sbufferConf);
	bufferConf = memset(bufferConf, '\0', sbufferConf);
	memcpy(bufferConf, &comando, scomando);
	memcpy(bufferConf + scomando, &fallo, sbool);
	memcpy(bufferConf + scomando + sbool, &map->mapID, sidmap);
	socket_send_packet(sock_marta, bufferConf, sbufferConf);
	free(bufferConf);
}
void confirmarMap(bool confirmacion, t_map* map) {

	/*TODO ARMAR METODO? */
	uint8_t comando = COMMAND_MAP;
	size_t scomando = sizeof(uint8_t);
	size_t sOrder = sizeof(bool);
	size_t sIdJob = sizeof(uint16_t);
	size_t sbuffer = scomando + sOrder + sIdJob;

	uint16_t idJob = htons(map->mapID);

	void* buffer = malloc(sbuffer);
	memset(buffer, '\0', sbuffer);
	memcpy(buffer, &comando, scomando);
	memcpy(buffer + scomando, &confirmacion, sOrder);
	memcpy(buffer + scomando + sOrder, &idJob, sIdJob);

	if (confirmacion == 1) {
		socket_send_packet(sock_marta, buffer, sbuffer);
		log_info(logger, "Map %d Successfully Completed", map->mapID);
	} else if (confirmacion == 0) {
		socket_send_packet(sock_marta, buffer, sbuffer);
		log_info(logger, "Map %d Failed", map->mapID);
	} else {
		log_error(logger, "Unknown command from nodo map confirm");
	}
	free(buffer);
	pthread_exit(NULL);

}

void atenderReducer(void * parametros) {
	//struct parms_threads *p = (struct parms_threads *) parametros;
	log_info(logger, "Thread reduce created");

	t_reduce *reduce = (t_reduce *) parametros;

	/* Desserializo el mensaje de Mapper de MaRTA */
	//
	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;
	int ret_val = 0;

	if ((sock_nodo = socket_connect(reduce->ip_nodo, reduce->port_nodo)) < 0) {
		log_error(logger, "Error al conectar reduce con Nodo %d", sock_nodo);
		failReduce(reduce);
		freeThreadReduce(reduce);
		pthread_exit(&ret_val);
	}

	log_info(logger, "Coneccion reduce con Nodo: %d", sock_nodo);

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO,
	HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger, "Error en reduce hand_nodo con Nodo %d", hand_nodo);
		freeThreadReduce(reduce);
		pthread_exit(&ret_val);
	}

	/* Serializo y mando datos */
	log_info(logger, "Handshake Reduce Nodo: %d", hand_nodo);
	serializeReduce(sock_nodo, reduce);

	/* Wait for confirmation */

	void *buffer;
	size_t sbuffer;
	e_socket_status status = socket_recv_packet(sock_nodo, &buffer, &sbuffer);

	/* Si se cae el nodo le mando que murio */
	if (status < 0) {
		failReduce(reduce);
		//free(buffer);
	} else {
		bool conf;
		size_t sConf = sizeof(bool);
		memcpy(&conf, buffer, sConf);

		/* Confirmar Map */
		confirmarReduce(conf, reduce, buffer);
	}
	free(buffer);
	socket_close(sock_nodo);
	freeThreadReduce(reduce);
	pthread_exit(&status);
}

void failReduce(t_reduce* reduce){
	uint8_t comando = COMMAND_REDUCE;
	size_t scomando = sizeof(uint8_t);
	size_t sbool = sizeof(bool);
	bool fallo = 0;
	size_t sidreduce = sizeof(uint16_t);
	size_t sbufferConf = scomando + sbool + sidreduce;
	void* bufferConf = malloc(sbufferConf);
	bufferConf = memset(bufferConf, '\0', sbufferConf);
	memcpy(bufferConf, &comando, scomando);
	memcpy(bufferConf + scomando, &fallo, sbool);
	memcpy(bufferConf + scomando + sbool, &reduce->reduceID, sidreduce);
	socket_send_packet(sock_marta, bufferConf, sbufferConf);
	free(bufferConf);
}
void confirmarReduce(char confirmacion, t_reduce* reduce, void* bufferNodo) {

	/*TODO ARMAR METODO? */
	uint8_t comando = COMMAND_REDUCE;
	size_t scomando = sizeof(uint8_t);
	size_t sOrder = sizeof(bool);
	size_t sIdJob = sizeof(uint16_t);
	uint16_t idJob = htons(reduce->reduceID);

	if (confirmacion == 1) {
		/* Armo el pkg reduce confirmation */
		size_t sbuffer = scomando + sOrder + sIdJob;
		void* buffer = malloc(sbuffer);
		buffer = memset(buffer, '\0', sbuffer);
		memcpy(buffer, &comando, scomando);
		memcpy(buffer + scomando, &confirmacion, sOrder);
		memcpy(buffer + scomando + sOrder, &idJob, sIdJob);
		/* Envio todo a MaRTA */
		socket_send_packet(sock_marta, &buffer, sbuffer);
		log_info(logger, "Reduce %d Successfully Completed", reduce->reduceID);
		free(buffer);
	} else if (confirmacion == 0) {
		/* Armo el pkg reduce confirmation */
		/*char* tmpfail;
		size_t stmpfail;

		memcpy(&stmpfail, bufferNodo + sIdJob, sizeof(size_t));
		tmpfail = malloc(stmpfail);
		memcpy(tmpfail, bufferNodo + sizeof(stmpfail), stmpfail);*/

		size_t sbuffer = scomando + sOrder + sIdJob + sizeof(size_t); //+ stmpfail;
		void* buffer = malloc(sbuffer);
		buffer = memset(buffer, '\0', sbuffer);
		memcpy(buffer, &comando, scomando);
		memcpy(buffer + scomando, &confirmacion, sOrder);
		memcpy(buffer + scomando + sOrder, &idJob, sIdJob);
		//memcpy(buffer + scomando + sOrder + sIdJob, &stmpfail, sizeof(size_t));
		//memcpy(buffer + scomando + sOrder + sIdJob + sizeof(size_t), &tmpfail, stmpfail);
		/* Envio todo a MaRTA */
		socket_send_packet(sock_marta, &buffer, sbuffer);
		log_info(logger, "Reduce %d Failed", reduce->reduceID);
	} else {
		log_error(logger, "Unknown Command from nodo confirm reduce");
	}
	free(bufferNodo);

}

//**********************************************************************************//
//									PAQUETES MaRTA 									//
//**********************************************************************************//

//***********************************MAP********************************************//
t_map* desserializeMapOrder(void *buffer) {
	size_t sIdMap = sizeof(uint16_t);
	size_t snumblock = sIdMap;
	size_t snodePort = sizeof(uint16_t);
	size_t snodeIP;
	uint16_t idMap;
	char* nodeIP;
	uint16_t nodePort = 0;
	uint16_t numBlock = 0;
	t_map* map;
	char tempResultName[60];
	memset(tempResultName, '\0', sizeof(char) * 60);

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(&tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, sizeof(char) * 60);

	//TODO GUARDAR EN ESTRUCTURAS
	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	map = malloc(sizeof(t_map));
	map->mapID = idMap;
	map->ip_nodo = strdup(nodeIP);
	map->port_nodo = nodePort;
	map->numBlock = numBlock;
	map->tempResultName = strdup(tempResultName);

	printf("\nID: %d\n", map->mapID);
	printf("IP:  %s  \n", map->ip_nodo);
	printf("PUERTO: %d\n", map->port_nodo);
	printf("NUMBLOCK: %d\n", map->numBlock);
	printf("%s\n", map->tempResultName);
	fflush(stdout);
	free(nodeIP);
	return (map);

}

//*********************************SerializeConfMap*******************************************//

//*********************************REDUCE*******************************************//
typedef struct {
	uint16_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char tempName[60];
} t_temp;

/*
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
*/

t_reduce* desserializeReduceOrder(void *buffer, size_t sbuffer) {
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;
	size_t sreduceID = sizeof(uint16_t);
	uint16_t reduceID;
	uint32_t stemps;

	char* nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
	//uint16_t countTemps = 0;

	memcpy(&reduceID, buffer, sreduceID);
	memcpy(&snodeIP, buffer + sreduceID, sizeof(size_t));
	//snodeIP = ntohs(snodeIP);
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sreduceID + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sreduceID + sizeof(snodeIP) + snodeIP, snodePort);
	nodePort = ntohs(nodePort);
	memcpy(tempResultName, buffer + sreduceID + sizeof(snodeIP) + snodeIP + snodePort, stempName);
	stemps = (sbuffer - (sreduceID + sizeof(snodeIP) + snodeIP + snodePort + stempName));
	void *buffer_tmps = malloc(stemps);
	memcpy(buffer_tmps, buffer + sreduceID + sizeof(snodeIP) + snodeIP + snodePort + stempName, stemps);

	/*
	memcpy(&countTemps, buffer + sreduceID + sizeof(snodeIP) + snodeIP + snodePort + stempName, sizeof(uint16_t));

	countTemps = ntohs(countTemps);
	void *tempsBuffer = malloc(sbuffer - sreduceID - sizeof(snodeIP) - snodeIP - snodePort - stempName - sizeof(uint16_t));
	tempsBuffer = buffer + sreduceID + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t);
	size_t stempsBuffer = 0;
	t_list *temps = list_create();
	for (; countTemps; countTemps--) {
		desserializeTempToList(temps, tempsBuffer, &stempsBuffer);
	}

	*/
	reduceID = ntohs(reduceID);

	t_reduce* reduce;
	reduce = malloc(sizeof(t_reduce));
	reduce->reduceID = reduceID;
	reduce->ip_nodo = strdup(nodeIP);
	reduce->port_nodo = nodePort;
	reduce->tempResultName = strdup(tempResultName);
	reduce->sizetmps = stemps;
	reduce->buffer_tmps = buffer_tmps;

	//reduce->tempResultName = strdup(tempResultName);
	//TODO VER COMO GUARDAR LA LISTA--Asi deberia funcionar joni
	//reduce->temps = temps;

//Test TODO: Adaptar a las estructuras de Job
/*
	printf("Count Temps:%d\n", list_size(temps));
	void showTemp(t_temp *temp) {
		printf("-Temp-\n");
		printf("\t%d\n", temp->originMap);
		printf("\t%s\n", temp->nodeIP);
		printf("\t%d\n", temp->nodePort);
		printf("\t%s\n", temp->tempName);
	}
	list_iterate(temps, (void *) showTemp);
	*/
	free(nodeIP);
	return (reduce);
//End
}
