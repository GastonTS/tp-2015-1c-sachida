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

void serializeMap(int sock_nodo, t_map* map){
	char order = 'm';
	size_t sOrder = sizeof(char);
	size_t sBlock = sizeof(uint16_t);
	char* fileMap;
	size_t sTempName = strlen(map->tempResultName);

	/* Obtenemos binario de File Map */
	fileMap = getMapReduceRoutine(cfgJob->MAPPER);
	size_t sfileMap = strlen(fileMap);

	/* htons */
	uint16_t numBlock = htons(map->numBlock);

	/* Armo el paquete y lo mando */
	size_t sbuffer = sOrder + sBlock + sizeof(sfileMap) + sfileMap + sizeof(sTempName) + sTempName;
	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &numBlock, sBlock);
	memcpy(buffer + sOrder + sBlock, &sfileMap, sizeof(sfileMap));
	memcpy(buffer + sOrder + sBlock + sizeof(sfileMap), fileMap, sfileMap);
	memcpy(buffer + sOrder + sBlock + sizeof(sfileMap) + sfileMap, &sTempName, sizeof(sTempName));
	memcpy(buffer + sOrder + sBlock + sizeof(sfileMap) + sfileMap + sizeof(sTempName), map->tempResultName, sTempName);

	int envio;
	envio = socket_send_packet(sock_nodo,buffer,sbuffer);
	log_info(logger,"Enviado %d",envio);
	log_info(logger,"Order: %c", order);
	log_info(logger,"numBlock: %d",numBlock);
	log_info(logger,"sfileMap: %d",sfileMap);
	log_info(logger,"fileMap: %s",fileMap);
	log_info(logger,"stemp: %d",sTempName);
	log_info(logger,"temp: %s",map->tempResultName);
	//free(fileMap);
	free(buffer);

}

//**********************************Send Reduce Nodo************************************//

void serializeReduce(int sock_nodo, t_reduce* reduce){
	char order = 'r';
	size_t sOrder = sizeof(char);
	char* fileReduce;
	size_t sTempName = strlen(reduce->tempResultName);

	/* Obtenemos binario de File Map */
	fileReduce = getMapReduceRoutine(cfgJob->REDUCER);
	size_t sfileReduce = strlen(fileReduce);

	/* htons */


	/* Armo el paquete y lo mando */
	size_t sbuffer = sOrder + sizeof(sfileReduce) + sfileReduce + sizeof(sTempName) + sTempName;
	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &sfileReduce, sizeof(sfileReduce));
	memcpy(buffer + sOrder + sizeof(sfileReduce), fileReduce, sfileReduce);
	memcpy(buffer + sOrder + sizeof(sfileReduce) + sfileReduce, &sTempName, sizeof(sTempName));
	memcpy(buffer + sOrder + sizeof(sfileReduce) + sfileReduce + sizeof(sTempName), reduce->tempResultName, sTempName);

	int envio;
	envio = socket_send_packet(sock_nodo,buffer,sbuffer);
	log_info(logger,"Enviado %d",envio);
	log_info(logger,"Order: %c", order);
	log_info(logger,"sfileMap: %d",sfileReduce);
	log_info(logger,"fileMap: %s",fileReduce);
	log_info(logger,"stemp: %d",sTempName);
	log_info(logger,"temp: %s",reduce->tempResultName);
	//free(fileMap);
	free(buffer);
}
