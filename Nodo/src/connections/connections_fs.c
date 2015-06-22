#include "connections_fs.h"
#include "connections.h"

void *connections_fs_connect(void *param);
void connections_fs_sendInfo();
void connections_fs_listenActions();
void connections_fs_deserializeSetBlock(void *buffer);
void connections_fs_deserializeGetBlock(void *buffer);

int fsSocket;
int exitFs;

void connections_fs_initialize() {
	exitFs = 0;
	fsSocket = -1;

	pthread_t fsTh;
	if (pthread_create(&fsTh, NULL, (void *) connections_fs_connect, NULL)) {
		log_error(node_logger, "Error while trying to create new thread: connections_fs_connect");
	}
	pthread_detach(fsTh);
}

void connections_fs_shutdown() {
	exitFs = 1;
}

void *connections_fs_connect(void *param) {

	while (!exitFs) {
		if (0 > fsSocket) {
			while (0 > fsSocket && !exitFs) {
				fsSocket = socket_connect(node_config->fsIp, node_config->fsPort);
			}
			if (fsSocket >= 0) {
				if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
					socket_close(fsSocket);
					fsSocket = -1;
					log_error(node_logger, "Handshake to filesystem failed.");
				} else {
					log_info(node_logger, "Connected to FileSystem.");
					connections_fs_sendInfo();
				}
			}
		}
	}
	socket_close(fsSocket);
	fsSocket = -1;
	return NULL;
}

void connections_fs_sendInfo() {

	uint8_t newNode = node_config->newNode;
	uint16_t blocksCount = node_config->blocksCount;

	uint16_t sName = strlen(node_config->name);
	size_t sBuffer = sizeof(newNode) + sizeof(blocksCount) + sizeof(sName) + sName + sizeof(node_config->listenPort);

	uint16_t blocksCountSerialized = htons(blocksCount);
	uint16_t listenPortSerialized = htons(node_config->listenPort);
	uint16_t sNameSerialized = htons(sName);

	void *pBuffer = malloc(sBuffer);
	memcpy(pBuffer, &newNode, sizeof(newNode));
	memcpy(pBuffer + sizeof(newNode), &blocksCountSerialized, sizeof(blocksCount));
	memcpy(pBuffer + sizeof(newNode) + sizeof(blocksCount), &listenPortSerialized, sizeof(listenPortSerialized));
	memcpy(pBuffer + sizeof(newNode) + sizeof(blocksCount) + sizeof(listenPortSerialized), &sNameSerialized, sizeof(sNameSerialized));
	memcpy(pBuffer + sizeof(newNode) + sizeof(blocksCount) + sizeof(listenPortSerialized) + sizeof(sName), node_config->name, sName);

	e_socket_status status = socket_send_packet(fsSocket, pBuffer, sBuffer);
	if (0 > status) {
		socket_close(fsSocket);
		fsSocket = -1;
	} else {
		connections_fs_listenActions();
	}

	free(pBuffer);
}

void connections_fs_listenActions() {
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
		case COMMAND_FS_TO_NODE_SET_BLOCK:
			connections_fs_deserializeSetBlock(buffer);
			break;
		case COMMAND_FS_TO_NODE_GET_BLOCK:
			connections_fs_deserializeGetBlock(buffer);
			break;
		default:
			log_error(node_logger, "FS Sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_fs_deserializeSetBlock(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sBlockData;
	memcpy(&sBlockData, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sBlockData = ntohl(sBlockData);

	char *blockData = malloc(sizeof(char) * (sBlockData + 1));
	memcpy(blockData, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sBlockData);
	blockData[sBlockData] = '\0';

	node_setBlock(numBlock, blockData);

	free(blockData);
}

void connections_fs_deserializeGetBlock(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	char *blockData = node_getBlock(numBlock);

	if (blockData) {
		e_socket_status status = socket_send_packet(fsSocket, blockData, strlen(blockData));

		if (status != SOCKET_ERROR_NONE) {
			// TODO, manejar el error.
		}
	} else {
		// TODO handlear error.
	}

	node_freeBlock(blockData);
}

