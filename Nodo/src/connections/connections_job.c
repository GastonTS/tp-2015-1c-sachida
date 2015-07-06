#include "connections_job.h"
#include "../node.h"
// TODO #include <commons/collections/list.h>

void* connections_job_listenActions(void *param);
void connections_job_deserializeMap(int socket, void *buffer);
void connections_job_deserializeReduce(int socket, void *buffer);

void connections_job_initialize() {

}

void connections_job_shutdown() {

}

void* connections_job_accept(void *param) {
	int *socketAcceptedPtr = (int *) param;

	log_info(node_logger, "New job connected.");

	pthread_t listenActionsTh;
	if (pthread_create(&listenActionsTh, NULL, (void *) connections_job_listenActions, (void *) socketAcceptedPtr)) {
		free(socketAcceptedPtr);
		log_error(node_logger, "Error while trying to create new thread: connections_job_listenActions");
	}
	pthread_detach(listenActionsTh);

	return NULL;
}

void* connections_job_listenActions(void *param) {
	int *socketAcceptedPtr = (int *) param;
	int socket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(socket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(socket);
			return NULL;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case COMMAND_MAP:
			connections_job_deserializeMap(socket, buffer);
			break;
		case COMMAND_REDUCE:
			connections_job_deserializeReduce(socket, buffer);
			break;
		default:
			log_error(node_logger, "JOB sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_job_deserializeMap(int socket, void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sMapRutine;
	memcpy(&sMapRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sMapRutine = ntohl(sMapRutine);

	char* mapRutine = malloc(sizeof(char) * (sMapRutine + 1));
	memcpy(mapRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sMapRutine);
	mapRutine[sMapRutine] = '\0';

	uint32_t sTmpName;
	memcpy(&sTmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sMapRutine, sizeof(uint32_t));
	sTmpName = ntohl(sTmpName);

	char* tmpName = malloc(sizeof(char) * (sTmpName + 1));
	memcpy(tmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sMapRutine + sizeof(uint32_t), sTmpName);
	tmpName[sTmpName] = '\0';

	log_info(node_logger, "Job Requested MAP Rutine. numBlock %d. tmpName: %s", numBlock, tmpName);

	bool ok = node_executeMapRutine(mapRutine, numBlock, tmpName);

	socket_send_packet(socket, &ok, sizeof(ok));

	free(mapRutine);
	free(tmpName);
}

void connections_job_deserializeReduce(int socket, void *buffer) {

	size_t offset = sizeof(uint8_t);

	uint32_t sReduceRutine;
	memcpy(&sReduceRutine, buffer + offset, sizeof(sReduceRutine));
	sReduceRutine = ntohl(sReduceRutine);
	offset += sizeof(sReduceRutine);

	char *reduceRutine = malloc(sizeof(char) * (sReduceRutine + 1));
	memcpy(reduceRutine, buffer + offset, sReduceRutine);
	reduceRutine[sReduceRutine] = '\0';
	offset += sReduceRutine;

	uint32_t sTmpName;
	memcpy(&sTmpName, buffer + offset, sizeof(sTmpName));
	sTmpName = ntohl(sTmpName);
	offset += sizeof(sTmpName);

	char *tmpName = malloc(sizeof(char) * (sTmpName + 1));
	memcpy(tmpName, buffer + offset, sTmpName);
	tmpName[sTmpName] = '\0';
	offset += sTmpName;

	// this is redirected from marta:

	uint16_t countTemps;
	memcpy(&countTemps, buffer + offset, sizeof(countTemps));
	countTemps = ntohs(countTemps);
	offset += sizeof(countTemps);

	int i;
	for (i = 0; i < countTemps; i++) {
		uint16_t sNodeId;
		char *nodeId;
		uint16_t sNodeIp;
		char *nodeIp;
		uint16_t nodePort;
		char *tmpName;

		memcpy(&sNodeId, buffer + offset, sizeof(sNodeId));
		sNodeId = ntohs(sNodeId);
		offset += sizeof(sNodeId);

		nodeId = malloc(sizeof(char) * (sNodeId + 1));
		memcpy(nodeId, buffer + offset, sNodeId);
		nodeId[sNodeId] = '\0';
		offset += sNodeId;

		memcpy(&sNodeIp, buffer + offset, sizeof(sNodeIp));
		sNodeIp = ntohs(sNodeIp);
		offset += sizeof(sNodeIp);

		nodeIp = malloc(sizeof(char) * (sNodeIp + 1));
		memcpy(nodeIp, buffer + offset, sNodeIp);
		nodeIp[sNodeIp] = '\0';
		offset += sNodeIp;

		memcpy(&nodePort, buffer + offset, sizeof(nodePort));
		nodePort = ntohs(nodePort);
		offset += sizeof(nodePort);

		tmpName = malloc(sizeof(char) * 60);
		memcpy(tmpName, buffer + offset, 60);

		log_info(node_logger, "Getting tmp file %s from node %s", tmpName, nodeId);
		// TODO tirar threads.
		if (strcmp(node_config->name, nodeId) == 0) {
			// it's me save to joined file.
			char *tmpFile = node_getTmpFileContent(tmpName);
			// TODO
			free(tmpFile);
		} else {
			// ask node the file content and save to joined file
			// TODO
		}
	}

	// from joined file
	// TODO bool ok = node_executeReduceRutine(reduceRutine, tmpName, )

	bool ok = 1;

	socket_send_packet(socket, &ok, sizeof(ok));

	free(reduceRutine);
	free(tmpName);

	// TODO free list items.
}
