#include "Connection.h"
#include "../Planning/MapPlanning.h"
#include "../Planning/ReducePlanning.h"
#include "../structs/node.h"
#include <commons/string.h>

//**********************************************************************************//
//									JOB												//
//**********************************************************************************//

void *acceptJob(void * param) {
	pthread_mutex_lock(&McantJobs);
	cantJobs++;
	pthread_mutex_unlock(&McantJobs);
	int *socketAcceptedPtr = (int *) param;
	int jobSocket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	t_job *job = desserializeJob(jobSocket, cantJobs);
	job->socket = jobSocket;

	if (job->combiner)
		log_info(logger, "Begin Job: %d (Combiner)", job->id);
	else
		log_info(logger, "Begin Job: %d (No combiner)", job->id);

	planMaps(job);

	if (job->combiner) {
		combinerReducePlanning(job);
	} else
		noCombinerReducePlanning(job);

	log_info(logger, "Finished Job: %d", job->id);
	sendDieOrder(job->socket, COMMAND_RESULT_OK);
	freeJob(job);
	return NULL;
}

void stringsToPathFile(t_list *list, char *string) {
	char **splits = string_split(string, " ");
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		t_file *file = CreateFile(*auxSplit);
		list_add(list, file);
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
}

t_job *desserializeJob(int socket, uint16_t id) {
	size_t sbuffer;
	void *buffer;
	e_socket_status status = socket_recv_packet(socket, &buffer, &sbuffer);
	if (0 > status) {
		log_error(logger, "Job %d Died when deserializing", id);
		free(buffer);
		pthread_exit(NULL);
	}

	bool combiner;
	uint16_t sresultadoFinal;

	memcpy(&combiner, buffer, sizeof(combiner));
	void *bufferOffset = buffer + sizeof(combiner);
	memcpy(&sresultadoFinal, bufferOffset, sizeof(sresultadoFinal));
	sresultadoFinal = ntohs(sresultadoFinal);
	char *resultadoFinal = malloc(sresultadoFinal + 1);
	bufferOffset += sizeof(sresultadoFinal);
	memcpy(resultadoFinal, bufferOffset, sresultadoFinal);
	resultadoFinal[sresultadoFinal] = '\0';
	size_t sfiles = sbuffer - sizeof(combiner) - sizeof(sresultadoFinal) - sresultadoFinal;
	char *stringFiles = malloc(sfiles + 1);
	bufferOffset += sresultadoFinal;
	memcpy(stringFiles, bufferOffset, sfiles);
	stringFiles[sfiles] = '\0';

	t_job *job = CreateJob(id, combiner, resultadoFinal);

	stringsToPathFile(job->files, stringFiles);

	free(stringFiles);
	free(buffer);
	return job;
}

char *recvResult(t_job *job) {
	void *buffer;
	char *nodeID = NULL;
	size_t sbuffer = 0;
	if (0 > socket_recv_packet(job->socket, &buffer, &sbuffer)) {
		log_error(logger, "Job %d Died when reciving results", job->id);
		freeJob(job);
		pthread_exit(NULL);
		return NULL;
	}

	uint8_t resultFrom;
	memcpy(&resultFrom, buffer, sizeof(resultFrom));
	printf("\n%d\n", resultFrom);
	fflush(stdout);
	switch (resultFrom) {
	case COMMAND_MAP:
		desserializeMapResult(buffer + sizeof(resultFrom), job);
		break;
	case COMMAND_REDUCE:
		nodeID = desserializaReduceResult(buffer + sizeof(resultFrom), job);
		break;
	}
	free(buffer);
	return nodeID;
}

e_socket_status sendDieOrder(int socket, uint8_t result) {
	uint8_t order = COMMAND_MARTA_TO_JOB_DIE;
	size_t sbuffer = sizeof(order) + sizeof(result);
	void *buffer = malloc(sbuffer);

	memcpy(buffer, &order, sizeof(order));
	memcpy(buffer + sizeof(order), &result, sizeof(result));

	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
	return status;
}
//**********************************MAP*********************************************//
e_socket_status serializeMapToOrder(int socket, t_map *map) {
	uint8_t order = COMMAND_MAP;
	uint16_t snodeIP = strlen(map->nodeIP);
	size_t sbuffer = sizeof(uint8_t) + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(char) * 60;

	uint16_t id = htons(map->id);
	uint16_t serializedSNodeIP = htons(snodeIP);
	uint16_t numBlock = htons(map->numBlock);
	uint16_t nodePort = htons(map->nodePort);

	void *buffer = malloc(sbuffer);
	memcpy(buffer, &order, sizeof(order));
	void *bufferOffset = buffer + sizeof(order);
	memcpy(bufferOffset, &id, sizeof(id));
	bufferOffset += sizeof(id);
	memcpy(bufferOffset, &serializedSNodeIP, sizeof(serializedSNodeIP));
	bufferOffset += sizeof(serializedSNodeIP);
	memcpy(bufferOffset, map->nodeIP, snodeIP);
	bufferOffset += snodeIP;
	memcpy(bufferOffset, &nodePort, sizeof(nodePort));
	bufferOffset += sizeof(nodePort);
	memcpy(bufferOffset, &numBlock, sizeof(numBlock));
	bufferOffset += sizeof(numBlock);
	memcpy(bufferOffset, map->tempResultName, sizeof(char) * 60);
	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(buffer);
	return status;
}

void desserializeMapResult(void *buffer, t_job *job) {
	bool result;
	uint16_t idMap;

	memcpy(&result, buffer, sizeof(result));
	memcpy(&idMap, buffer + sizeof(result), sizeof(idMap));
	idMap = ntohs(idMap);

	log_trace(logger, "Map: %d Done -> Result: %d", idMap, result);
	bool findMap(t_map *map) {
		return isMap(map, idMap);
	}
	t_map *map = list_find(job->maps, (void *) findMap);

	removeMapNode(map);
	if (result) {
		map->done = true;
	} else {
		deactivateNode(map->nodeName);
		rePlanMap(job, map);
	}
}

//*********************************REDUCE*******************************************//
size_t totalTempsSize(t_list *temps) {
	size_t stemps = 0;
	void upgradeSize(t_temp * temp) {
		uint16_t snodeID = strlen(temp->nodeID);
		uint16_t snodeIP = strlen(temp->nodeIP);
		stemps += sizeof(snodeID) + snodeID + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
	}
	list_iterate(temps, (void *) upgradeSize);
	return stemps;
}

void serializeTemp(t_temp *temporal, void *buffer, size_t *sbuffer) {
	uint16_t snodeID = strlen(temporal->nodeID);
	uint16_t snodeIP = strlen(temporal->nodeIP);
	uint16_t nodePort = htons(temporal->nodePort);

	uint16_t serializedsnodeID = htons(snodeID);
	uint16_t serializedsnodeIP = htons(snodeIP);

	void *bufferOffset = buffer + *sbuffer;
	memcpy(bufferOffset, &serializedsnodeID, sizeof(serializedsnodeID));
	bufferOffset += sizeof(serializedsnodeID);
	memcpy(bufferOffset, temporal->nodeID, snodeID);
	bufferOffset += snodeID;
	memcpy(bufferOffset, &serializedsnodeIP, sizeof(serializedsnodeIP));
	bufferOffset += sizeof(serializedsnodeIP);
	memcpy(bufferOffset, temporal->nodeIP, snodeIP);
	bufferOffset += snodeIP;
	memcpy(bufferOffset, &nodePort, sizeof(nodePort));
	bufferOffset += sizeof(nodePort);
	memcpy(bufferOffset, temporal->tempName, sizeof(char) * 60);

	*sbuffer += sizeof(serializedsnodeID) + snodeID + sizeof(serializedsnodeIP) + snodeIP + sizeof(nodePort) + sizeof(char) * 60;
}

e_socket_status serializeReduceToOrder(int socket, t_reduce *reduce) {
	uint8_t order = COMMAND_REDUCE;
	uint16_t snodeIP = strlen(reduce->nodeIP);

	uint16_t countTemps = 0;
	size_t stemps = totalTempsSize(reduce->temps);
	size_t auxSize = 0;
	void *tempsBuffer = malloc(stemps);
	void serializeTempsToBuffer(t_temp *temp) {
		serializeTemp(temp, tempsBuffer, &auxSize);
		countTemps++;
	}
	list_iterate(reduce->temps, (void *) serializeTempsToBuffer);

	uint16_t reduceID = htons(reduce->id);
	uint16_t nodePort = htons(reduce->nodePort);
	uint16_t serializedSNodeIP = htons(snodeIP);
	countTemps = htons(countTemps);

	size_t sbuffer = sizeof(order) + sizeof(reduceID) + sizeof(serializedSNodeIP) + snodeIP + sizeof(nodePort) + sizeof(char) * 60 + sizeof(countTemps)
			+ stemps;
	void *buffer = malloc(sbuffer);

	memcpy(buffer, &order, sizeof(order));
	void *bufferOffset = buffer + sizeof(order);
	memcpy(bufferOffset, &reduceID, sizeof(reduceID));
	bufferOffset += sizeof(reduceID);
	memcpy(bufferOffset, &serializedSNodeIP, sizeof(serializedSNodeIP));
	bufferOffset += sizeof(serializedSNodeIP);
	memcpy(bufferOffset, reduce->nodeIP, snodeIP);
	bufferOffset += snodeIP;
	memcpy(bufferOffset, &nodePort, sizeof(nodePort));
	bufferOffset += sizeof(nodePort);
	memcpy(bufferOffset, reduce->tempResultName, sizeof(char) * 60);
	bufferOffset += sizeof(char) * 60;
	memcpy(bufferOffset, &countTemps, sizeof(countTemps));
	bufferOffset += sizeof(countTemps);
	memcpy(bufferOffset, tempsBuffer, stemps);

	e_socket_status status = socket_send_packet(socket, buffer, sbuffer);
	free(tempsBuffer);
	free(buffer);
	return status;
}

char *desserializaReduceResult(void *buffer, t_job *job) {
	bool result;
	uint16_t idReduce;
	memcpy(&result, buffer, sizeof(result));
	void *bufferOffset = buffer + sizeof(result);
	memcpy(&idReduce, bufferOffset, sizeof(idReduce));
	idReduce = ntohs(idReduce);

	t_reduce *reduce;
	if (!idReduce)
		reduce = job->finalReduce;
	else {
		bool findReduce(t_reduce *reduce) {
			return isReduce(reduce, idReduce);
		}
		reduce = list_find(job->partialReduces, (void *) findReduce);
	}
	log_trace(logger, "Reduce: %d Done -> Result: %d", idReduce, result);
	removeReduceNode(reduce);
	if (result) {
		reduce->done = 1;
		return NULL;
	} else {
		bufferOffset += sizeof(idReduce);
		uint16_t snodeID;
		memcpy(&snodeID, bufferOffset, sizeof(snodeID));
		snodeID = ntohs(snodeID);
		char *nodeID = malloc(snodeID + 1);
		bufferOffset += sizeof(snodeID);
		memcpy(nodeID, bufferOffset, snodeID);
		nodeID[snodeID] = '\0';
		if (strcmp(nodeID, "ErrorAlConectar")) {
			free(nodeID);
			nodeID = strdup(reduce->finalNode);
		}
		deactivateNode(nodeID);
		return nodeID;
	}

}
