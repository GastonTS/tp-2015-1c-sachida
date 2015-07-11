/*
 * node.c
 *
 *  Created on: 23/6/2015
 *      Author: utnso
 */
#include "node.h"
#include "Job.h"
#include "../utils/socket.h"

//**********************************************************************************//
//									PAQUETES NODO 									//
//**********************************************************************************//

//**********************************Send Map Nodo************************************//

void serializeMap(int sock_nodo, t_map* map) {
	uint8_t order = COMMAND_MAP;
	size_t sOrder = sizeof(uint8_t);
	size_t sBlock = sizeof(uint16_t);
	char* fileMap;

	/* Obtenemos binario de File Map */
	fileMap = getMapReduceRoutine(cfgJob->MAPPER);
	uint32_t sfileMap = strlen(fileMap);
	uint32_t sTempName = strlen(map->tempResultName);
	size_t sbuffer = sOrder + sBlock + sizeof(uint32_t) + sfileMap
			+ sizeof(uint32_t) + sTempName;

	/* htons - htonl */
	uint16_t numBlock = htons(map->numBlock);

	uint32_t sfileMapSerialize = htonl(sfileMap);
	uint32_t sTempNameSerialize = htonl(sTempName);

	/* Armo el paquete y lo mando */

	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &numBlock, sBlock);
	memcpy(buffer + sOrder + sBlock, &sfileMapSerialize, sizeof(uint32_t));
	memcpy(buffer + sOrder + sBlock + sizeof(uint32_t), fileMap, sfileMap);
	memcpy(buffer + sOrder + sBlock + sizeof(uint32_t) + sfileMap,
			&sTempNameSerialize, sizeof(uint32_t));
	memcpy(
			buffer + sOrder + sBlock + sizeof(uint32_t) + sfileMap
					+ sizeof(uint32_t), map->tempResultName, sTempName);

	e_socket_status status = socket_send_packet(sock_nodo, buffer, sbuffer);
	if (status > 0) {
		log_info(logger, "Map: %d --> send to nodo", map->mapID);
	}
	//free(fileMap);
	free(buffer);

}

//**********************************Send Reduce Nodo************************************//

void serializeReduce(int sock_nodo, t_reduce* reduce) {
	uint8_t order = COMMAND_REDUCE;
	size_t sOrder = sizeof(uint8_t);
	char* fileReduce;
	uint32_t sTempName = strlen(reduce->tempResultName);

	/* Obtenemos binario de File Map */
	fileReduce = getMapReduceRoutine(cfgJob->REDUCER);
	uint32_t sfileReduce = strlen(fileReduce);

	/* htons */
	uint32_t sfileReduceSerialize = htonl(sfileReduce);
	uint32_t sTempNameSerialize = htonl(sTempName);

	/* Armo el paquete y lo mando */
	size_t sbuffer = sOrder + sizeof(uint32_t) + sfileReduce + sizeof(uint32_t)
			+ sTempName + sizeof(uint32_t) + reduce->sizetmps;
	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &sfileReduceSerialize, sizeof(uint32_t));
	memcpy(buffer + sOrder + sizeof(uint32_t), fileReduce, sfileReduce);
	memcpy(buffer + sOrder + sizeof(uint32_t) + sfileReduce,
			&sTempNameSerialize, sizeof(uint32_t));
	memcpy(buffer + sOrder + sizeof(uint32_t) + sfileReduce + sizeof(uint32_t),
			reduce->tempResultName, sTempName);
	memcpy(
			buffer + sOrder + sizeof(uint32_t) + sfileReduce + sizeof(uint32_t)
					+ sTempName, reduce->buffer_tmps, reduce->sizetmps);


	e_socket_status status = socket_send_packet(sock_nodo, buffer, sbuffer);

	if (status > 0) {
		log_info(logger, "Reduce: %d --> send to nodo", reduce->reduceID);
	}
	//free(fileMap);
	free(buffer);
}
