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
		// TODO avisar a marta que no existe el archivo.
		return 1;
	}

	// |cantbloques [cantCopias [sizeNodeId|nodeId|blockIndex] ]
	uint16_t blocksCount = list_size(file->blocks);
	uint16_t blocksCountSerialized = htons(blocksCount);

	void *buffer = malloc(sizeof(blocksCount));
	memcpy(buffer, &blocksCountSerialized, sizeof(blocksCount));

	size_t sBuffer = sizeof(blocksCount);

	void listBlocks(t_list* blockCopies) {
		uint16_t copyesCount = 0;
		void *blockCopiesBuffer = NULL;
		size_t sBlockCopiesBuffer = 0;

		void listBlockCopy(file_block_t *blockCopy) {
			if (connections_node_isActiveNode(blockCopy->nodeId)) {
				copyesCount++;

				uint32_t sNode = strlen(blockCopy->nodeId);
				size_t sBlockCopy = sizeof(sNode) + sNode + sizeof(blockCopy->blockIndex);
				blockCopiesBuffer = realloc(blockCopiesBuffer, sBlockCopiesBuffer + sBlockCopy);

				uint32_t sNodeSerialized = htonl(sNode);
				uint16_t blockIndexSerialized = htons(blockCopy->blockIndex);

				memcpy(blockCopiesBuffer + sBlockCopiesBuffer, &sNodeSerialized, sizeof(sNode));
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNode), blockCopy->nodeId, sNode);
				memcpy(blockCopiesBuffer + sBlockCopiesBuffer + sizeof(sNode) + sNode, &blockIndexSerialized, sizeof(blockCopy->blockIndex));

				sBlockCopiesBuffer += sBlockCopy;
			}
		}

		list_iterate(blockCopies, (void *) listBlockCopy);

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

	e_socket_status status = socket_send_packet(martaSocket, buffer, sBuffer);

	free(buffer);

	return (status == SOCKET_ERROR_NONE);
}
