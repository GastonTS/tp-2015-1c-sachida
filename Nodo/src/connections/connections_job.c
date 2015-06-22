#include "connections_job.h"
#include "connections.h"

void *connections_job_connect(int socket);
void connections_job_sendInfo();
void connections_job_listenActions();
void connections_job_deserializeMap(void *buffer);
void connections_job_deserializeReduce(void *buffer);

int jobSocket;
int exitJob;

void connections_job_initialize(t_nodeCfg *config) {
	return;
	// TODO ...

	exitJob = 0;
	jobSocket = -1;
	pthread_t jobTh;
	if (pthread_create(&jobTh, NULL, (void *) connections_job_connect, (void *) config)) {
		log_error(node_logger, "Error while trying to create new thread: connections_job_connect");
	}
	pthread_detach(jobTh);
}

void connections_job_shutdown() {
	exitJob = 1;
}

void *connections_job_connect(int socketAccepted) {

	while (!exitJob) {
		if (0 > jobSocket) {
			while (0 > jobSocket && !exitJob) {
				jobSocket = socket_listen(socketAccepted);
			}
			if (jobSocket >= 0) {
				if (HANDSHAKE_FILESYSTEM != socket_handshake_to_client(jobSocket, HANDSHAKE_NODO, HANDSHAKE_JOB)) {
					socket_close(jobSocket);
					jobSocket = -1;
					log_error(node_logger, "Handshake to Job failed.");
				} else {
					log_info(node_logger, "Connected to Job.");

				}
			}
		}
	}
	socket_close(jobSocket);
	jobSocket = -1;
	return NULL;
}

void connections_job_sendInfo() {
	//TODO ver si aca manda informacion, o recibe
}

void connections_job_listenActions() {
	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(jobSocket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(jobSocket);
			jobSocket = -1; // TODO mover a metodo y meter mutex por fsSocket.
			return;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case 3: //setBloque // TODO mover a socket.h
			connections_job_deserializeMap(buffer);
			break;
		case 4: //getBloque
			connections_job_deserializeReduce(buffer);
			break;
		default:
			log_error(node_logger, "JOB Sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_job_deserializeMap(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint16_t sizeMap;
	memcpy(&sizeMap, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint16_t));
	sizeMap = ntohl(sizeMap);

	char* map = malloc(sizeof(char) * (sizeMap));
	memcpy(map, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), sizeMap);

	uint32_t sizeTmp;
	memcpy(&sizeTmp, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap, sizeof(uint16_t));
	sizeTmp = ntohl(sizeTmp);

	char* tmp = malloc(sizeof(char) * (sizeTmp));
	memcpy(tmp, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap + sizeof(uint16_t), sizeTmp);

	//node_setBlock(numBlock, blockData);
	//node_map();
	printf("llego al deseralize map\n");
	free(tmp);
	free(map);
}

void connections_job_deserializeReduce(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint16_t sizeReduce;
	memcpy(&sizeReduce, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint16_t));
	sizeReduce = ntohl(sizeReduce);

	char* reduce = malloc(sizeof(char) * (sizeReduce));
	memcpy(reduce, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), sizeReduce);
	//node_reduce();
	printf("llego al deseralize reduce\n");
	free(reduce);
}
