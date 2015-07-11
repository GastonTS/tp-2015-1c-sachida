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
void serializeConfigMaRTA(int fd, bool combiner, char* fileResult, char* stringFiles) {
	uint16_t filesLength = strlen(stringFiles);
	uint16_t sizeResult = strlen(fileResult);

	/* Htons */
	uint16_t sizeResultSerialize = htons(sizeResult);
	combiner = htons(combiner);

	size_t sbuffer = sizeof(combiner) + sizeof(uint16_t) + sizeResult + filesLength;
	void *buffer = malloc(sbuffer);

	memcpy(buffer, &combiner, sizeof(combiner));
	void *bufferOffset = buffer + sizeof(combiner);
	memcpy(bufferOffset, &sizeResultSerialize, sizeof(sizeResultSerialize));
	bufferOffset += sizeof(sizeResultSerialize);
	memcpy(bufferOffset, fileResult, sizeResult);
	bufferOffset += sizeResult;
	memcpy(bufferOffset, stringFiles, filesLength);

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
	else if (finalResult == COMMAND_RESULT_CANT_COPY)
		log_info(logger, "Job Failed: Canto copy final result to MDFS");
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
	memcpy(&order, buffer, sizeof(order));

	printf("\n Orden Rcv: %d\n", order);
	fflush(stdout);
	if (order == COMMAND_MAP) {
		log_info(logger, "Map Recived");
		t_map *map = malloc(sizeof(t_map));
		map = desserializeMapOrder(buffer + sizeof(order));
		pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, (void *) map);
		pthread_detach(hilo_mapper);

	} else if (order == COMMAND_REDUCE) {
		log_info(logger, "Reduce Recived");
		t_reduce *reduce = malloc(sizeof(t_reduce));
		reduce = desserializeReduceOrder(buffer + sizeof(order), sbuffer - sizeof(order));
		pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, (void *) reduce);
		pthread_detach(hilo_reduce);
	}

	else if (order == COMMAND_MARTA_TO_JOB_DIE) {
		desserializeDieOrder(buffer + sizeof(order));
		freeCfg();
		free(buffer);
		exit(0);
	}

	free(buffer);
}

void atenderMapper(void * parametros) {
	t_map *map = (t_map *) parametros;
	log_info(logger, "Map: %d NEW --> Thread created", map->mapID);

	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;

	if ((sock_nodo = socket_connect(map->ip_nodo, map->port_nodo)) < 0) {
		log_error(logger, "Map: %d ERROR --> connecting to Nodo %d", map->mapID, sock_nodo);
		sendMapResult(map, 0);
		freeThreadMap(map);
		pthread_exit(&sock_nodo);
		return;
	}

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO,
	HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger, "Map: %d ERROR --> hand_nodo con Nodo %d", map->mapID, hand_nodo);
		freeThreadMap(map);
		pthread_exit(&hand_nodo);
		return;
	}

	/* Serializo y mando datos */
	serializeMap(sock_nodo, map);

	/* Wait for confirmation */
	void *buffer;
	size_t sbuffer;
	e_socket_status status = socket_recv_packet(sock_nodo, &buffer, &sbuffer);

	/* Si se cae el nodo le mando que murio */
	if (status < 0) {
		sendMapResult(map, 0);
	} else {
		bool result;
		memcpy(&result, buffer, sizeof(result));
		sendMapResult(map, result);
		free(buffer);
	}
	socket_close(sock_nodo);
	freeThreadMap(map);
	pthread_exit(&status);

}

void sendMapResult(t_map* map, bool result) {
	uint8_t comando = COMMAND_MAP;

	uint16_t mapID = htons(map->mapID);

	size_t sbuffer = sizeof(comando) + sizeof(result) + sizeof(mapID);
	void* buffer = malloc(sbuffer);

	memcpy(buffer, &comando, sizeof(comando));
	void *bufferOffset = buffer + sizeof(comando);
	memcpy(bufferOffset, &result, sizeof(result));
	bufferOffset += sizeof(result);
	memcpy(bufferOffset, &mapID, sizeof(mapID));

	pthread_mutex_lock(&Msockmarta);
	socket_send_packet(sock_marta, buffer, sbuffer);
	pthread_mutex_unlock(&Msockmarta);
	free(buffer);
	log_info(logger, "Map: %d Done -> Result: %d", map->mapID, result);
}

void atenderReducer(void * parametros) {
	t_reduce *reduce = (t_reduce *) parametros;
	log_info(logger, "Reduce: %d NEW --> Thread created", reduce->reduceID);

	/* Me conecto al Nodo */
	int hand_nodo;
	int sock_nodo;

	if ((sock_nodo = socket_connect(reduce->ip_nodo, reduce->port_nodo)) < 0) {
		log_error(logger, "Reduce: %d ERROR --> to connect reduce con Nodo %d", reduce->reduceID, sock_nodo);
		char *errorAlConectar = strdup("ErrorAlConectar");
		failReduce(reduce, errorAlConectar);
		freeThreadReduce(reduce);
		pthread_exit(&sock_nodo);
	}

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO,
	HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger, "Reduce: %d ERROR --> hand_nodo con Nodo %d", reduce->reduceID, hand_nodo);
		failReduce(reduce, "ErrorAlConectar");
		freeThreadReduce(reduce);
		pthread_exit(&hand_nodo);
	}

	/* Serializo y mando datos */
	serializeReduce(sock_nodo, reduce);
	/* Wait for confirmation */
	void *buffer;
	size_t sbuffer;
	e_socket_status status = socket_recv_packet(sock_nodo, &buffer, &sbuffer);

	/* Si se cae el nodo le mando que murio */
	if (status < 0) {
		char *errorAlConectar = strdup("ErrorAlConectar");
		failReduce(reduce, errorAlConectar);
		//free(buffer);
		log_error(logger, "Reduce: %d ERROR --> Fail Recv Nodo", reduce->reduceID);
	} else {
		bool result;
		memcpy(&result, buffer, sizeof(result));
		confirmarReduce(reduce, result, buffer, sizeof(result));
		log_info(logger, "Reduce: %d OK --> finalize", reduce->reduceID);
	}
	socket_close(sock_nodo);
	freeThreadReduce(reduce);
	pthread_exit(&status);
}

void failReduce(t_reduce* reduce, char *fallenNode) {
	uint8_t comando = COMMAND_REDUCE;
	bool fallo = 0;
	uint16_t sfallenNode = strlen(fallenNode);

	uint16_t reduceID = htons(reduce->reduceID);
	uint16_t serializedSFallenNode = htons(sfallenNode);

	size_t sbufferConf = sizeof(comando) + sizeof(bool) + sizeof(reduceID) + sizeof(serializedSFallenNode) + sfallenNode;
	void* bufferConf = malloc(sbufferConf);

	memcpy(bufferConf, &comando, sizeof(comando));
	void *bufferOffset = bufferConf + sizeof(comando);
	memcpy(bufferOffset, &fallo, sizeof(bool));
	bufferOffset += sizeof(bool);
	memcpy(bufferOffset, &reduceID, sizeof(reduceID));
	bufferOffset += sizeof(reduceID);
	memcpy(bufferOffset, &serializedSFallenNode, sizeof(serializedSFallenNode));
	bufferOffset += sizeof(serializedSFallenNode);
	memcpy(bufferOffset, fallenNode, sfallenNode);

	pthread_mutex_lock(&Msockmarta);
	socket_send_packet(sock_marta, bufferConf, sbufferConf);
	pthread_mutex_unlock(&Msockmarta);
	free(fallenNode);
	free(bufferConf);
}

void confirmarReduce(t_reduce* reduce, bool result, void* bufferNodo, size_t bufferOffset) {
	uint8_t comando = COMMAND_REDUCE;
	uint16_t reduceID = htons(reduce->reduceID);

	if (result) {
		size_t sbuffer = sizeof(comando) + sizeof(result) + sizeof(reduceID);
		void* buffer = malloc(sbuffer);

		memcpy(buffer, &comando, sizeof(comando));
		void *bufferOffset = buffer + sizeof(comando);
		memcpy(bufferOffset, &result, sizeof(result));
		bufferOffset += sizeof(result);
		memcpy(bufferOffset, &reduceID, sizeof(reduceID));

		pthread_mutex_lock(&Msockmarta);
		socket_send_packet(sock_marta, buffer, sbuffer);
		pthread_mutex_unlock(&Msockmarta);
		free(buffer);
		free(bufferNodo);
	} else {
		uint16_t snodeID;
		memcpy(&snodeID, bufferNodo + bufferOffset, sizeof(snodeID));
		bufferOffset += sizeof(snodeID);
		snodeID = ntohs(snodeID);

		char *nodeID = malloc(snodeID + 1);
		memcpy(nodeID, bufferNodo + bufferOffset, snodeID);
		nodeID[snodeID] = '\0';
		log_info(logger, "nodeIDFail: %s", nodeID);
		free(bufferNodo);
		failReduce(reduce, nodeID);
	}

}

//**********************************************************************************//
//									PAQUETES MaRTA 									//
//**********************************************************************************//

//***********************************MAP********************************************//
t_map* desserializeMapOrder(void *buffer) {
	uint16_t sIdMap = sizeof(uint16_t);
	uint16_t snumblock = sIdMap;
	uint16_t snodePort = sizeof(uint16_t);
	uint16_t snodeIP;
	uint16_t idMap;
	char* nodeIP;
	uint16_t nodePort = 0;
	uint16_t numBlock = 0;
	t_map* map;
	char tempResultName[60];
	memset(tempResultName, '\0', sizeof(char) * 60);

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(uint16_t));
	snodeIP = ntohs(snodeIP);
	nodeIP = malloc(snodeIP + 1);
	memcpy(nodeIP, buffer + sIdMap + sizeof(uint16_t), snodeIP);
	nodeIP[snodeIP] = '\0';
	memcpy(&nodePort, buffer + sIdMap + sizeof(uint16_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(uint16_t) + snodeIP + snodePort, snumblock);
	memcpy(&tempResultName, buffer + sIdMap + sizeof(uint16_t) + snodeIP + snodePort + snumblock, sizeof(char) * 60);

	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	map = malloc(sizeof(t_map));
	map->mapID = idMap;
	map->ip_nodo = strdup(nodeIP);
	map->port_nodo = nodePort;
	map->numBlock = numBlock;
	map->tempResultName = strdup(tempResultName);

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

t_reduce* desserializeReduceOrder(void *buffer, size_t sbuffer) {
	uint16_t snodePort = sizeof(uint16_t);
	uint16_t stempName = sizeof(char) * 60;
	uint16_t snodeIP;
	uint16_t sreduceID = sizeof(uint16_t);
	uint16_t reduceID;
	uint32_t stemps;

	char* nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
//uint16_t countTemps = 0;

	memcpy(&reduceID, buffer, sreduceID);
	memcpy(&snodeIP, buffer + sreduceID, sizeof(uint16_t));
	snodeIP = ntohs(snodeIP);
	nodeIP = malloc(snodeIP + 1);
	memcpy(nodeIP, buffer + sreduceID + sizeof(uint16_t), snodeIP);
	nodeIP[snodeIP] = '\0';
	memcpy(&nodePort, buffer + sreduceID + sizeof(uint16_t) + snodeIP, snodePort);
	nodePort = ntohs(nodePort);
	memcpy(tempResultName, buffer + sreduceID + sizeof(uint16_t) + snodeIP + snodePort, stempName);
	stemps = (sbuffer - (sreduceID + sizeof(uint16_t) + snodeIP + snodePort + stempName));
	void *buffer_tmps = malloc(stemps);
	memcpy(buffer_tmps, buffer + sreduceID + sizeof(uint16_t) + snodeIP + snodePort + stempName, stemps);

	reduceID = ntohs(reduceID);

	t_reduce* reduce;
	reduce = malloc(sizeof(t_reduce));
	reduce->reduceID = reduceID;
	reduce->ip_nodo = strdup(nodeIP);
	reduce->port_nodo = nodePort;
	reduce->tempResultName = strdup(tempResultName);
	reduce->sizetmps = stemps;
	reduce->buffer_tmps = buffer_tmps;

	free(nodeIP);
	return (reduce);
}
