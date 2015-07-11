#include "connections_job.h"
#include "connections_node.h"
#include "../node.h"
#include <commons/collections/list.h>

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

	sem_wait(&routines_sem);

	pthread_t listenActionsTh;
	if (pthread_create(&listenActionsTh, NULL, (void *) connections_job_listenActions, (void *) socketAcceptedPtr)) {
		socket_close(*socketAcceptedPtr);
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

	size_t sBuffer;
	void *buffer = NULL;

	e_socket_status status = socket_recv_packet(socket, &buffer, &sBuffer);
	if (status == SOCKET_ERROR_NONE) {
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

	socket_close(socket);
	sem_post(&routines_sem);
	return NULL;
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

	char *finalTmpName = malloc(sizeof(char) * (sTmpName + 1));
	memcpy(finalTmpName, buffer + offset, sTmpName);
	finalTmpName[sTmpName] = '\0';
	offset += sTmpName;

	// this is redirected from marta:

	uint16_t countTemps;
	memcpy(&countTemps, buffer + offset, sizeof(countTemps));
	countTemps = ntohs(countTemps);
	offset += sizeof(countTemps);

	char *tmpFileNameToReduce = NULL;
	t_list *getTmpFileOperations = list_create();

	int i;
	for (i = 0; i < countTemps; i++) {
		uint16_t sNodeId;
		char *nodeId;
		uint16_t sNodeIp;
		char *nodeIp;
		uint16_t nodePort;
		char *tmpFileName;

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

		tmpFileName = malloc(sizeof(char) * 60);
		memcpy(tmpFileName, buffer + offset, sizeof(char) * 60);
		offset += sizeof(char) * 60;

		if (countTemps == 1 && strcmp(node_config->name, nodeId) == 0) {
			// Just one tmpFile and it's mine, then should use that..
			tmpFileNameToReduce = strdup(tmpFileName);
		} else {
			node_connection_getTmpFileOperation_t *operation = node_connection_getTmpFileOperation_create(nodeId, nodeIp, nodePort, tmpFileName);
			list_add(getTmpFileOperations, operation);
		}
		free(tmpFileName);
		free(nodeId);
		free(nodeIp);
	}

	char *nodeIdFailed = NULL;
	if (!tmpFileNameToReduce) {
		t_list *tmpFileParts = list_create();
		pthread_mutex_t tmpFileParts_mutex;
		if (pthread_mutex_init(&tmpFileParts_mutex, NULL) != 0) {
			log_error(node_logger, "Error while trying to create new mutex: tmpFileParts_mutex");
			return;
		}
		bool failed = 0;
		void addPartToList(char *part, char *nodeId) {
			pthread_mutex_lock(&tmpFileParts_mutex);
			if (!part) {
				failed = 1;
				nodeIdFailed = strdup(nodeId);
			}
			list_add(tmpFileParts, part);
			pthread_mutex_unlock(&tmpFileParts_mutex);
		}
		void* getTmpFileFromNode(char *param) {
			node_connection_getTmpFileOperation_t *operation = (node_connection_getTmpFileOperation_t *) param;
			char *tmpFile = NULL;
			if (strcmp(node_config->name, operation->nodeId) == 0) { // it's me
				log_info(node_logger, "Getting tmp file %s from me!", operation->tmpFileName);
				tmpFile = node_getTmpFileContent(operation->tmpFileName);
			} else {
				log_info(node_logger, "Getting tmp file %s from node %s", operation->tmpFileName, operation->nodeId);
				tmpFile = connections_node_getFileContent(operation);
			}
			addPartToList(tmpFile, operation->nodeId);
			return NULL;
		}
		pthread_t threads[list_size(getTmpFileOperations)];
		int count = 0;
		void runOperations(node_connection_getTmpFileOperation_t *operation) {
			if (!failed) {
				if (pthread_create(&(threads[count]), NULL, (void *) getTmpFileFromNode, (void*) operation)) {
					failed = 1;
					log_error(node_logger, "Error while trying to create new thread: getTmpFileFromNode");
				} else {
					count++;
				}
			}
		}
		list_iterate(getTmpFileOperations, (void *) runOperations);
		int i;
		for (i = 0; i < count; i++) {
			pthread_join(threads[i], NULL);
		}
		pthread_mutex_destroy(&tmpFileParts_mutex);

		// Join to one file (to be reduced then..)
		if (failed) {
			log_error(node_logger, "Couldn't get all the files to make a reduce. Failed Node %s..", nodeIdFailed);
		} else {
			char *tmpFileNameJoined = malloc(sizeof(char) * strlen(finalTmpName) + 20);
			strcpy(tmpFileNameJoined, finalTmpName);
			strcat(tmpFileNameJoined, "_prereduce_joined");
			// TODO hace sort si no no anda nada !!
			if (node_createTmpFileFromStringList(tmpFileNameJoined, tmpFileParts)) {
				tmpFileNameToReduce = tmpFileNameJoined;
			} else {
				log_error(node_logger, "Couldn't create joined file..");
			}
		}
		list_destroy_and_destroy_elements(tmpFileParts, (void*) free);
	}
	list_destroy_and_destroy_elements(getTmpFileOperations, (void*) node_connection_getTmpFileOperation_free);

	bool ok = 0;
	if (tmpFileNameToReduce) {
		ok = node_executeReduceRutine(reduceRutine, tmpFileNameToReduce, finalTmpName);
		free(tmpFileNameToReduce);
	}

	void *bufferResponse;
	size_t sBufferResponse;
	if (!ok) {
		if (!nodeIdFailed) {
			nodeIdFailed = node_config->name;
		}
		uint16_t sNodeId = strlen(nodeIdFailed);
		uint16_t sNodeIdSerialized = htons(sNodeId);

		sBufferResponse = sizeof(ok) + sizeof(sNodeId) + sNodeId;
		bufferResponse = malloc(sBufferResponse);

		memcpy(bufferResponse, &ok, sizeof(ok));
		memcpy(bufferResponse + sizeof(ok), &sNodeIdSerialized, sizeof(sNodeId));
		memcpy(bufferResponse + sizeof(ok) + sizeof(sNodeId), nodeIdFailed, sNodeId);
		free(nodeIdFailed);
	} else {
		sBufferResponse = sizeof(ok);
		bufferResponse = malloc(sBufferResponse);
		memcpy(bufferResponse, &ok, sizeof(ok));
	}

	socket_send_packet(socket, bufferResponse, sBufferResponse);
	free(bufferResponse);

	free(reduceRutine);
	free(finalTmpName);
}

