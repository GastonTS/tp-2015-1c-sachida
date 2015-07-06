#include "connections_fs.h"

void connections_fs_setDisconnected();
void *connections_fs_connect(void *param);
void connections_fs_sendInfo();
void connections_fs_listenActions();
void connections_fs_deserializeSetBlock(void *buffer);
void connections_fs_deserializeGetBlock(void *buffer);

int fsSocket;
int exitFsConnections;
pthread_t fsTh;

void connections_fs_initialize() {
	exitFsConnections = 0;
	fsSocket = -1;

	if (pthread_create(&fsTh, NULL, (void *) connections_fs_connect, NULL)) {
		log_error(node_logger, "Error while trying to create new thread: connections_fs_connect");
	}
}

void connections_fs_shutdown() {
	exitFsConnections = 1;
	connections_fs_setDisconnected();
	pthread_join(fsTh, NULL);
}

void connections_fs_setDisconnected() {
	socket_close(fsSocket);
	fsSocket = -1;
}

void connections_fs_setConnectionLost() {
	log_error(node_logger, "Connection to FS was lost.");
	connections_fs_setDisconnected();
}

void *connections_fs_connect(void *param) {

	while (!exitFsConnections) {
		if (0 > fsSocket) {
			while (0 > fsSocket && !exitFsConnections) {
				fsSocket = socket_connect(node_config->fsIp, node_config->fsPort);
			}
			if (fsSocket >= 0) {
				if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
					log_error(node_logger, "Handshake to filesystem failed.");
					connections_fs_setDisconnected();
				} else {
					log_info(node_logger, "Connected to FileSystem.");
					connections_fs_sendInfo();
				}
			}
		}
	}

	connections_fs_setDisconnected();
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
		connections_fs_setConnectionLost();
	} else {
		connections_fs_listenActions();
	}

	free(pBuffer);
}

void connections_fs_listenActions() {
	while (!exitFsConnections) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(fsSocket, &buffer, &sBuffer);
		if (0 > status) {
			connections_fs_setConnectionLost();
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
		case COMMAND_NODE_GET_TMP_FILE_CONTENT:
			connections_deserializeGetFileContent(fsSocket, buffer);
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
			connections_fs_setConnectionLost();
		}
		free(blockData);
	} else {
		// Should not happen EVER.
		log_error(node_logger, "Cannot get block %d. OMG.", numBlock);
	}
}

