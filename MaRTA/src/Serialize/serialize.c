#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include "../../../utils/socket.h"
#include "serialize.h"
#include <arpa/inet.h>

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

t_job *desserealizeJob(int fd, uint32_t id) {
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

void serialiceMapToOrder(int fd, t_map *map) {
	char order = 'm';
	size_t sIpMap = sizeof(uint32_t);
	size_t snumBlock = sIpMap;
	size_t snodePort = sizeof(uint16_t);
	size_t sOrder = sizeof(char);
	size_t snodeIP = strlen(map->nodeIP) + 1;
	size_t stempName = sizeof(char) * 60;
	size_t sbuffer = sOrder + sIpMap + sizeof(snodeIP) + snodeIP + snumBlock + snodePort + stempName;
	void *buffer = malloc(sbuffer);

	uint32_t id = htonl(map->id);
	uint32_t numBlock = htonl(map->numBlock);
	uint16_t nodePort = htons(map->nodePort);

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
