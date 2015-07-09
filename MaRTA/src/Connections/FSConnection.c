#include "Connection.h"
#include "../structs/job.h"
#include "../structs/node.h"

void connectFS();

int fsSocket;

void initFSConnection() {
	fsSocket = -1;
	connectFS();
}

void FSConnectionLost() {
	log_error(logger, "Connection to FS was lost.");
	socket_close(fsSocket);
	fsSocket = -1;
	connectFS();
}

void connectFS() {
	if (0 > fsSocket) {
		log_info(logger, "Connecting MDFS...");
		while (0 > fsSocket) {
			fsSocket = socket_connect(cfgMaRTA->fsIP, cfgMaRTA->fsPort);
		}
		if (fsSocket >= 0) {
			if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA)) {
				log_error(logger, "Handshake to filesystem failed.");
				FSConnectionLost();
			} else {
				log_info(logger, "Connected to FileSystem.");
			}
		}
	}
}

void updateNodes(char* nodeID, char *nodeIP, uint16_t nodePort) {
	t_node *node = findNode(nodes, nodeID);
	if (node != NULL) {
		free(node->ip);
		node->ip = strdup(nodeIP);
		node->port = nodePort;
		node->active = 1;
	} else {
		node = CreateNode(1, nodeIP, nodePort, nodeID);
		list_add(nodes, node);
	}
}

int requestFileBlocks(t_file *file) {
	void *buffer;
	void request() {
		uint8_t command = COMMAND_MARTA_TO_FS_GET_FILE_BLOCKS;
		uint32_t sfilePath = strlen(file->path);

		size_t sBuffer = sizeof(command) + sizeof(sfilePath) + sfilePath;

		uint32_t sSerializedFile = htonl(sfilePath);

		buffer = malloc(sBuffer);
		memcpy(buffer, &command, sizeof(command));
		memcpy(buffer + sizeof(command), &sSerializedFile, sizeof(sfilePath));
		memcpy(buffer + sizeof(command) + sizeof(sfilePath), file->path, sfilePath);

		while (0 > socket_send_packet(fsSocket, buffer, sBuffer)) {
			FSConnectionLost();
		}
		free(buffer);
	}
	request();

	size_t sbuffer = 0;
	while (0 > socket_recv_packet(fsSocket, &buffer, &sbuffer)) {
		FSConnectionLost();
		request();
	}

	uint16_t blocksCount;
	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	if (blocksCount == 0) {
		log_error(logger, "File: %s not available", file->path);
		free(buffer);
		return 0;
	}
	void *bufferOffset = buffer + sizeof(blocksCount);

	int fileBlockNumber;
	for (fileBlockNumber = 0; fileBlockNumber < blocksCount; fileBlockNumber++) {
		t_list *copies = list_create();
		uint16_t copyesCount;
		memcpy(&copyesCount, bufferOffset, sizeof(copyesCount));
		copyesCount = ntohs(copyesCount);
		bufferOffset += sizeof(copyesCount);
		int j;
		for (j = 0; j < copyesCount; j++) {
			uint32_t sNodeId;
			uint16_t sNodeIp;
			uint16_t nodePort;
			uint16_t nodeBlockNumber;

			memcpy(&sNodeId, bufferOffset, sizeof(sNodeId));
			sNodeId = ntohl(sNodeId);
			bufferOffset += sizeof(sNodeId);

			char *nodeId = malloc(sNodeId + 1);
			memcpy(nodeId, bufferOffset, sNodeId);
			nodeId[sNodeId] = '\0';
			bufferOffset += sNodeId;

			memcpy(&sNodeIp, bufferOffset, sizeof(sNodeIp));
			sNodeIp = ntohs(sNodeIp);
			bufferOffset += sizeof(sNodeIp);

			char * nodeIp = malloc(sNodeIp + 1);
			memcpy(nodeIp, bufferOffset, sNodeIp);
			nodeIp[sNodeIp] = '\0';
			bufferOffset += sNodeIp;

			memcpy(&nodePort, bufferOffset, sizeof(nodePort));
			nodePort = ntohs(nodePort);
			bufferOffset += sizeof(nodePort);

			memcpy(&nodeBlockNumber, bufferOffset, sizeof(nodeBlockNumber));
			nodeBlockNumber = ntohs(nodeBlockNumber);
			bufferOffset += sizeof(nodeBlockNumber);

			pthread_mutex_lock(&Mnodes);
			updateNodes(nodeId, nodeIp, nodePort);
			pthread_mutex_unlock(&Mnodes);

			t_copy *copy = CreateCopy(nodeId, nodeBlockNumber);

			list_add(copies, copy);

			free(nodeId);
			free(nodeIp);
		}
		list_add(file->blocks, copies);

	}
	free(buffer);
	return 1;
}

bool copyFinalTemporal(t_job *job) {
	uint8_t command = COMMAND_MARTA_TO_FS_GET_COPY_FINAL_RESULT;
	uint16_t sarchivoResultado = strlen(job->resultadoFinal);

	uint16_t serializedsresult = htons(sarchivoResultado);

	size_t sbuffer = sizeof(sarchivoResultado) + sizeof(command) + sarchivoResultado + sizeof(char) * 60;
	void *buffer = malloc(sbuffer);

	memcpy(buffer, &command, sizeof(command));
	void *bufferOffset = buffer + sizeof(command);
	memcpy(buffer, &serializedsresult, sizeof(serializedsresult));
	bufferOffset += sizeof(serializedsresult);
	memcpy(bufferOffset, job->resultadoFinal, sarchivoResultado);
	bufferOffset += sarchivoResultado;
	memcpy(bufferOffset, job->finalReduce->tempResultName, sizeof(char) * 60);

	while (0 > socket_send_packet(fsSocket, buffer, sbuffer)) {
		FSConnectionLost();
	}
	void *finalBuffer;
	size_t sfinalBuffer;
	while (0 > socket_recv_packet(fsSocket, &finalBuffer, &sfinalBuffer)) {
		FSConnectionLost();
	}
	bool result;
	memcpy(&result, finalBuffer, sizeof(result));
	free(finalBuffer);
	return result;
}

