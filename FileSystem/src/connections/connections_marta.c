#include "connections_node.h"
#include "connections_marta.h"
#include "connections.h"
#include "../filesystem/filesystem.h"

#include <commons/collections/list.h>

void *connections_marta_listenActions(void *param);
bool connections_marta_sendFileBlocks(void *bufferReceived);
bool connections_marta_copyFinalResult(void *buffer);

int martaSocket;

void connections_marta_initialize() {
	martaSocket = -1;
}

void connections_marta_shutdown() {
	socket_close(martaSocket);
}

void* connections_marta_accept(void *param) {
	int *socketAcceptedPtr = (int *) param;
	int socketAccepted = *socketAcceptedPtr;
	free(socketAcceptedPtr);

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
			log_error(mdfs_logger, "MaRTA was disconnected");
			exit = 1;
		} else {
			uint8_t command;
			memcpy(&command, buffer, sizeof(uint8_t));

			switch (command) {
			case COMMAND_MARTA_TO_FS_GET_FILE_BLOCKS:
				if (!connections_marta_sendFileBlocks(buffer)) {
					log_error(mdfs_logger, "MaRTA was disconnected");
					exit = 1;
				}
				break;
			case COMMAND_MARTA_TO_FS_GET_COPY_FINAL_RESULT:
				connections_marta_copyFinalResult(buffer);
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

bool connections_marta_sendFileBlocks(void *bufferReceived) {
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
			node_connection_t* nodeConnection = connections_node_getActiveNodeConnection(blockCopy->nodeId);
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

				size_t offset = 0;
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, &sNodeIdSerialized, sizeof(sNodeId));
				offset += sizeof(sNodeId);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, blockCopy->nodeId, sNodeId);
				offset += sNodeId;
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, &sNodeIpSerialized, sizeof(sNodeIp));
				offset += sizeof(sNodeIp);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, nodeConnection->ip, sNodeIp);
				offset += sNodeIp;
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, &nodePortSerialized, sizeof(nodeConnection->listenPort));
				offset += sizeof(nodeConnection->listenPort);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + offset, &blockIndexSerialized, sizeof(blockCopy->blockIndex));

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

bool connections_marta_copyFinalResult(void *bufferReceived) {
	uint16_t sNodeId;
	uint16_t sResultFileName;
	char finalTmpName[60];

	size_t offset = sizeof(uint8_t);

	memcpy(&sNodeId, bufferReceived + offset, sizeof(sNodeId));
	sNodeId = ntohs(sNodeId);
	offset += sizeof(sNodeId);

	char *nodeId = malloc(sNodeId + 1);
	memcpy(nodeId, bufferReceived + offset, sNodeId);
	nodeId[sNodeId] = '\0';
	offset += sNodeId;

	memcpy(&sResultFileName, bufferReceived + offset, sizeof(sResultFileName));
	sResultFileName = ntohs(sResultFileName);
	offset += sizeof(sResultFileName);

	char *resultFileName = malloc(sResultFileName + 1);
	memcpy(resultFileName, bufferReceived + offset, sResultFileName);
	resultFileName[sResultFileName] = '\0';
	offset += sResultFileName;

	memcpy(finalTmpName, bufferReceived + offset, sizeof(char) * 60);

	uint8_t failReason = COMMAND_RESULT_CANT_COPY;
	bool result = filesystem_copyTmpFileToMDFS(nodeId, finalTmpName, resultFileName, &failReason);
	free(nodeId);
	free(resultFileName);
	size_t sbuffer = sizeof(result);
	void *buffer;
	if (result) {
		buffer = malloc(sbuffer);
		memcpy(buffer, &result, sizeof(result));
	} else {
		sbuffer += sizeof(failReason);
		buffer = malloc(sbuffer);
		memcpy(buffer, &result, sizeof(result));
		memcpy(buffer + sizeof(result), &failReason, sizeof(failReason));
	}

	e_socket_status status = socket_send_packet(martaSocket, buffer, sbuffer);
	free(buffer);

	return status == SOCKET_ERROR_NONE;
}
