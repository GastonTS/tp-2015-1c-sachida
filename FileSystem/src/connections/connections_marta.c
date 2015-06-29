#include "connections_node.h"
#include "connections_marta.h"
#include "connections.h"
#include "../filesystem/filesystem.h"

#include <commons/collections/list.h>

void *connections_marta_listenActions(void *param);
bool connection_marta_sendFileBlocks(void *bufferReceived);

int martaSocket;

void connections_marta_initialize() {
	martaSocket = -1;
}

void connections_marta_shutdown() {
	socket_close(martaSocket);
}

void* connections_marta_accept(void *param) {
	int socketAccepted = *(int *) param;

	if (martaSocket != -1) {
		log_error(mdfs_logger, "Marta tried to connect but rejected because there was another Marta connected");
		socket_close(socketAccepted);
		return NULL;
	}

	log_info(mdfs_logger, "Marta connected.");

	pthread_t martaTh;
	martaSocket = socketAccepted;
	if (pthread_create(&martaTh, NULL, (void *) connections_marta_listenActions, NULL)) {
		log_error(mdfs_logger, "Error while trying to create new thread: connections_marta_listenActions");
	}
	pthread_detach(martaTh);

	return NULL;
}

void *connections_marta_listenActions(void *param) {
	int exit = 0;

	while (!exit) {
		void *buffer = NULL;
		size_t sBuffer = 0;

		e_socket_status status = socket_recv_packet(martaSocket, &buffer, &sBuffer);

		if (status != SOCKET_ERROR_NONE) {
			exit = 1;
		} else {
			uint8_t command;
			memcpy(&command, buffer, sizeof(uint8_t));

			switch (command) {
			case COMMAND_MARTA_TO_FS_GET_FILE_BLOCKS:
				if (!connection_marta_sendFileBlocks(buffer)) {
					exit = 1;
				}
				break;
			}
		}

		if (buffer) {
			free(buffer);
		}
	}
	socket_close(martaSocket);
	martaSocket = -1;
	return NULL;
}

bool connection_marta_sendFileBlocks(void *bufferReceived) {
	bool sendFailed() {
		uint16_t blocksCount = 0; // We send 0 as an error.
		uint16_t blocksCountSerialized = htons(blocksCount);

		void *buffer = malloc(sizeof(blocksCount));
		memcpy(buffer, &blocksCountSerialized, sizeof(blocksCount));

		size_t sBuffer = sizeof(blocksCount);

		e_socket_status status = socket_send_packet(martaSocket, buffer, sBuffer);
		free(buffer);

		bool success = status == SOCKET_ERROR_NONE;
		log_info(mdfs_logger, "Error informed to MaRTA %s!", success ? "successfully" : "unsuccessfully");

		return success;
	}

	uint32_t sFileName;
	memcpy(&sFileName, bufferReceived + sizeof(uint8_t), sizeof(uint32_t));
	sFileName = ntohl(sFileName);

	char *filePathName = malloc(sizeof(char) * (sFileName + 1));
	memcpy(filePathName, bufferReceived + sizeof(uint8_t) + sizeof(uint32_t), sFileName);
	filePathName[sFileName] = '\0';

	log_info(mdfs_logger, "Marta requested the blocks of file %s", filePathName);

	file_t *file = filesystem_resolveFilePath(filePathName, ROOT_DIR_ID, "/");

	free(filePathName);

	if (!file) {
		log_error(mdfs_logger, "The file that marta requested was not found.");
		return sendFailed();
	}

	bool fileAvailable = 1;

	// |cantbloques [cantCopias [sizeNodeId|nodeId|blockIndex] ]
	uint16_t blocksCount = list_size(file->blocks);
	uint16_t blocksCountSerialized = htons(blocksCount);

	void *buffer = malloc(sizeof(blocksCount));
	memcpy(buffer, &blocksCountSerialized, sizeof(blocksCount));

	size_t sBuffer = sizeof(blocksCount);

	void listBlocks(t_list* blockCopies) {
		if (!fileAvailable) {
			return;
		}

		uint16_t copyesCount = 0;
		void *blockCopiesBuffer = NULL;
		size_t sBlockCopiesBuffer = 0;

		void listBlockCopies(file_block_t *blockCopy) {
			node_connection_t* nodeConnection = connections_node_getNodeConnection(blockCopy->nodeId);
			if (nodeConnection) {
				copyesCount++;

				uint32_t sNodeId = strlen(blockCopy->nodeId);
				uint16_t sNodeIp = strlen(nodeConnection->ip);
				size_t sBlockCopy = sizeof(sNodeId) + sNodeId + sizeof(sNodeIp) + sNodeIp + sizeof(nodeConnection->listenPort) + sizeof(blockCopy->blockIndex);
				blockCopiesBuffer = realloc(blockCopiesBuffer, sBlockCopiesBuffer + sBlockCopy);

				uint32_t sNodeIdSerialized = htonl(sNodeId);
				uint16_t sNodeIpSerialized = htons(sNodeIp);
				uint16_t nodePortSerialized = htons(nodeConnection->listenPort);
				uint16_t blockIndexSerialized = htons(blockCopy->blockIndex);

				memcpy(blockCopiesBuffer + sBlockCopiesBuffer, &sNodeIdSerialized, sizeof(sNodeId));
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNodeId), blockCopy->nodeId, sNodeId);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNodeId) + sNodeId, &sNodeIpSerialized, sizeof(sNodeIp));
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNodeId) + sNodeId + sizeof(sNodeIp), nodeConnection->ip, sNodeIp);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNodeId) + sNodeId + sizeof(sNodeIp) + sNodeIp, &nodePortSerialized, sizeof(nodeConnection->listenPort));
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNodeId) + sNodeId + sizeof(sNodeIp) + sNodeIp + sizeof(nodeConnection->listenPort), &blockIndexSerialized, sizeof(blockCopy->blockIndex));

				sBlockCopiesBuffer += sBlockCopy;
			}
		}
		list_iterate(blockCopies, (void *) listBlockCopies);
		if (copyesCount == 0) {
			fileAvailable = 0;
			return;
		}

		uint16_t copyesCountSerialized = htons(copyesCount);

		buffer = realloc(buffer, sBuffer + sizeof(copyesCount));
		memcpy(buffer + sBuffer, &copyesCountSerialized, sizeof(copyesCount));
		sBuffer += sizeof(copyesCount);

		buffer = realloc(buffer, sBuffer + sBlockCopiesBuffer);
		memcpy(buffer + sBuffer, blockCopiesBuffer, sBlockCopiesBuffer);
		sBuffer += sBlockCopiesBuffer;

		free(blockCopiesBuffer);
	}
	list_iterate(file->blocks, (void *) listBlocks);
	file_free(file);

	if (!fileAvailable) {
		log_error(mdfs_logger, "The file that marta requested is not available because one or more of it's copies are down.");
		free(buffer);
		return sendFailed();
	}

	e_socket_status status = socket_send_packet(martaSocket, buffer, sBuffer);
	free(buffer);

	bool success = status == SOCKET_ERROR_NONE;
	log_info(mdfs_logger, "Blocks sent %s!", success ? "successfully" : "unsuccessfully");

	return success;
}
