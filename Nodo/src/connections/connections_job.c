#include "connections_job.h"
#include "connections.h"

void *connections_job_connect(void *param);
void connections_job_sendInfo(t_nodeCfg *config);
void connections_job_listenActions();

int jobSocket;
int exitJob;

void connections_job_initialize(t_nodeCfg *config) {
	exitFs = 0;
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

void *connections_job_connect(void *param) {
	t_nodeCfg *config = (t_nodeCfg *) param;

	while (!exitFs) {
		if (0 > jobSocket) {
			while (0 > jobSocket && !exitFs) {
				jobSocket = socket_listen(config->puerto_job);
			}
			if (jobSocket >= 0) {
				if (HANDSHAKE_FILESYSTEM != socket_handshake_to_client(jobSocket, HANDSHAKE_NODO, HANDSHAKE_JOB)) {
					socket_close(jobSocket);
					jobSocket = -1;
					log_error(node_logger, "Handshake to Job failed.");
				} else {
					log_info(node_logger, "Connected to Job.");
					connections_job_sendInfo(config);
				}
			}
		}
	}
	socket_close(fsSocket);
	fsSocket = -1;
	return NULL;
}

void connections_job_sendInfo(t_nodeCfg *config) {
	// TODO from config.
	uint16_t cantBloques = 30;
	char myName[] = "Nodo1"; // Le paso mi nombre.

	uint16_t sName = strlen(myName);
	size_t sBuffer = sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(sName) + sName + sizeof(config->puerto_nodo);

	uint16_t cantBloquesSerialized = htons(cantBloques);
	uint16_t puertoNodoSerialized = htons(config->puerto_nodo);
	uint16_t sNameSerialized = htons(sName);

	void *pBuffer = malloc(sBuffer);
	memcpy(pBuffer, &config->nodo_nuevo, sizeof(config->nodo_nuevo));
	memcpy(pBuffer + sizeof(config->nodo_nuevo), &cantBloquesSerialized, sizeof(cantBloques));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques), &puertoNodoSerialized, sizeof(puertoNodoSerialized));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(puertoNodoSerialized), &sNameSerialized, sizeof(sNameSerialized));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(puertoNodoSerialized) + sizeof(sName), &myName, sName);

	e_socket_status status = socket_send_packet(fsSocket, pBuffer, sBuffer);
	if (0 > status) {
		socket_close(fsSocket);
		fsSocket = -1;
	} else {
		connections_fs_listenActions();
	}

	free(pBuffer);
}

void connections_job_listenActions() {
	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(fsSocket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(fsSocket);
			fsSocket = -1; // TODO mover a metodo y meter mutex por fsSocket.
			return;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case 1: //setBloque // TODO mover a socket.h
			connections_job_deserializeMap(buffer);
			break;
		case 2: //getBloque
			connections_job_deserializeReduce(buffer);
			break;
		default:
			log_error(node_logger, "FS Sent an invalid command %d", command);
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
	memcpy(map, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), map);

	uint32_t sizeTmp;
	memcpy(&sizeTmp,buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap, sizeof(uint16_t));
	sizeTmp = ntohl(sizeTmp);

	char* tmp = malloc(sizeof(char) * (sizeTmp));
	memcpy(tmp, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap + sizeof(uint16_t), sizeTmp);

	//node_setBlock(numBlock, blockData);
	//node_map();
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
	memcpy(reduce, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), reduce);
	//node_reduce();
	free(reduce);
}
