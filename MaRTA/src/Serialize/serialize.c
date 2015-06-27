#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "../../../utils/socket.h"
#include "serialize.h"
#include "../Planning/MapPlanning.h"
#include <arpa/inet.h>

//**********************************************************************************//
//									JOB												//
//**********************************************************************************//
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

t_job *desserealizeJob(int fd, uint16_t id) {
	size_t sbuffer;
	void *buffer;
	socket_recv_packet(fd, &buffer, &sbuffer);

	size_t scombiner = sizeof(bool);
	bool combiner;
	size_t sfiles = sbuffer - scombiner;
	char *stringFiles = malloc(sfiles);

	memcpy(&combiner, buffer, scombiner);
	memcpy(stringFiles, (buffer + scombiner), sfiles);

	t_job *job = CreateJob(id, combiner);
	stringsToPathFile(job->files, stringFiles);

	free(stringFiles);
	free(buffer);
	return job;
}

void recvResult(int fd, t_job *job) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	char resultFrom = '\0';
	size_t sResult = sizeof(char);
	memcpy(&resultFrom, buffer, sResult);
	printf("\nRECVRESULT: %c\n", resultFrom);
	if (resultFrom == COMMAND_MAP)
		desserializeMapResult(buffer + sResult, job);
	else if (resultFrom == COMMAND_REDUCE)
		desserializaReduceResult(buffer + sResult, job);
	free(buffer);
}

void sendDieOrder(int fd) {
	char order = 'd';
	size_t sOrder = sizeof(char);
	size_t sbuffer = sOrder;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}
//**********************************MAP*********************************************//
void serializeMapToOrder(int fd, t_map *map) {
	uint8_t order = COMMAND_MAP;
	size_t sOrder = sizeof(uint8_t);
	size_t sIpMap = sizeof(uint16_t);
	size_t snumBlock = sIpMap;
	size_t snodePort = sizeof(uint16_t);
	size_t snodeIP = strlen(map->nodeIP) + 1;
	size_t stempName = sizeof(char) * 60;
	size_t sbuffer = sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock + stempName;

	uint16_t id = htons(map->id);
	uint16_t numBlock = htons(map->numBlock);
	uint16_t nodePort = htons(map->nodePort);

	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &id, sIpMap);
	memcpy(buffer + sOrder + sIpMap, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP), map->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort, &numBlock, snumBlock);
	memcpy(buffer + sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snodePort + snumBlock, map->tempResultName, stempName);
	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}

void desserializeMapResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidMap = sizeof(uint16_t);

	bool result;
	uint16_t idMap;

	memcpy(&result, buffer, sresult);
	memcpy(&idMap, buffer + sresult, sidMap);
	idMap = ntohs(idMap);

	bool findMap(t_map *map) {
		return isMap(map, idMap);
	}
	t_map *map = list_find(job->maps, (void *) findMap);

	if (result) {
		map->done = true;
	} else {
		rePlanMap(job, map);
	}
}
//*********************************REDUCE*******************************************//
size_t totalTempsSize(t_list *temps) {
	size_t stemps = 0;
	void upgradeSize(t_temp * temp) {
		size_t snodeIP = strlen(temp->nodeIP) + 1;
		stemps += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
	}
	list_iterate(temps, (void *) upgradeSize);
	return stemps;
}

void serializeTemp(t_temp *temporal, void *buffer, size_t *sbuffer) {
	size_t snodeIP = strlen(temporal->nodeIP) + 1;
	uint16_t originMap = htons(temporal->originMap);
	uint16_t nodePort = htons(temporal->nodePort);

	memcpy(buffer + *sbuffer, &originMap, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(uint16_t), &snodeIP, sizeof(snodeIP));
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP), temporal->nodeIP, snodeIP);
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP, &nodePort, sizeof(uint16_t));
	memcpy(buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), temporal->tempName, sizeof(char) * 60);

	*sbuffer += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

void serializeReduceToOrder(int fd, t_reduce *reduce) {
	char order = COMMAND_REDUCE;
	size_t sOrder = sizeof(char);
	size_t snodeIP = strlen(reduce->nodeIP) + 1;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;

	uint16_t nodePort = htons(reduce->nodePort);

	uint16_t countTemps = 0;
	size_t stemps = totalTempsSize(reduce->temps);
	size_t auxSize = 0;
	void *tempsBuffer = malloc(stemps);
	void serializeTempsToBuffer(t_temp *temp) {
		serializeTemp(temp, tempsBuffer, &auxSize);
		countTemps++;
	}
	list_iterate(reduce->temps, (void *) serializeTempsToBuffer);
	countTemps = htons(countTemps);

	size_t sbuffer = sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t) + stemps;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &snodeIP, sizeof(snodeIP));
	memcpy(buffer + sOrder + sizeof(snodeIP), reduce->nodeIP, snodeIP);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP, &nodePort, snodePort);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort, reduce->tempResultName, stempName);
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName, &countTemps, sizeof(uint16_t));
	memcpy(buffer + sOrder + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t), tempsBuffer, stemps);

	socket_send_packet(fd, buffer, sbuffer);
	free(tempsBuffer);
	free(buffer);
}

void desserializaReduceResult(void *buffer, t_job *job) {
	size_t sresult = sizeof(bool);
	size_t sidReduce = sizeof(uint16_t);

	bool result;
	uint16_t idReduce;
	char failedTemp[60];
	memset(failedTemp, '\0', sizeof(char) * 60);

	memcpy(&result, buffer, sresult);
	memcpy(&idReduce, buffer + sresult, sidReduce);
	memcpy(&failedTemp, buffer + sresult + sidReduce, sizeof(char) * 60);
	idReduce = ntohs(idReduce);

	if (result) {
		if (!idReduce)
			job->finalReduce->done = 1;
		else {
			bool findReduce(t_reduce *reduce) {
				return isReduce(reduce, idReduce);
			}
			t_reduce *reduce = list_find(job->partialReduces, (void *) findReduce);
			reduce->done = 1;
		}

	} else {
		//TODO RePlanReduce
		printf("%s\n", failedTemp);
	}
}
