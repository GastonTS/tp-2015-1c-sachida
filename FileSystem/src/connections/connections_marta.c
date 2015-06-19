#include "connections_node.h"
#include "connections_marta.h"
#include "connections.h"
#include "../filesystem/filesystem.h"

#include <commons/collections/list.h>

void *connections_marta_listenActions(void *param);
bool connection_marta_sendFileBlocks(void *bufferReceived);

int martaSocket;

void connections_marta_initialize() {

}

void connections_marta_shutdown() {

}

void connections_marta_accept(int socketAccepted) {
	martaSocket = socketAccepted;

	pthread_t martaTh; // TODO TEMA CONNECTIONS, deberia ir global para terminarlo o q?

	if (pthread_create(&martaTh, NULL, (void *) connections_marta_listenActions, NULL)) {
		log_error(mdfs_logger, "Error while trying to create new thread: connections_marta_listenActions");
	}
}

void *connections_marta_listenActions(void *param) {

	void *buffer;
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet(martaSocket, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	uint8_t command;
	memcpy(&command, buffer, sizeof(uint8_t));

	switch (command) {
	case MARTA_COMMAND_GET_FILE_BLOCKS:
		connection_marta_sendFileBlocks(buffer);
		break;
	}

	free(buffer);
	return NULL;
}

bool connection_marta_sendFileBlocks(void *bufferReceived) {
	uint32_t sFileName;
	memcpy(&sFileName, bufferReceived + sizeof(uint8_t), sizeof(uint32_t));
	sFileName = ntohl(sFileName);

	char *fileName = malloc(sizeof(char) * (sFileName + 1));
	memcpy(fileName, bufferReceived + sizeof(uint8_t) + sizeof(uint32_t), sFileName);
	fileName[sFileName] = '\0';

	log_info(mdfs_logger, "Marta requested the blocks of file %s", fileName);
	file_t *file = filesystem_resolveFilePath(fileName, ROOT_DIR_ID, "/");

	if (!file) {
		// TODO  Return code =  that the is no such file to marta?
		return 0;
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
