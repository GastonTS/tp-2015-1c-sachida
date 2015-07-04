#include "connections_job.h"
#include "../node.h"

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
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sReduceRutine; // TODO , avisar a JOB que esto va a ser de 32 bits
	memcpy(&sReduceRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sReduceRutine = ntohl(sReduceRutine);

	char* reduceRutine = malloc(sizeof(char) * (sReduceRutine + 1));
	memcpy(reduceRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sReduceRutine);
	reduceRutine[sReduceRutine] = '\0';

	// TODO bool ok = node_executeReduceRutine(reduceRutine, numBlock);

	bool ok = 1;

	socket_send_packet(socket, &ok, sizeof(ok));

	free(reduceRutine);
}
