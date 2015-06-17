#include "connections_node.h"
#include "connections.h"

#include <commons/collections/dictionary.h>

int connections_node_getNodeSocket(char *nodeId);
void connections_node_setNodeSocket(char *nodeId, int socket);

t_dictionary *nodesSockets;

void connections_node_initialize() {
	nodesSockets = dictionary_create();
}

void connections_node_shutdown() {
	void desconectNode(int *nodeSocket) {
		socket_close(*nodeSocket);
		free(nodeSocket);
	}
	dictionary_destroy_and_destroy_elements(nodesSockets, (void*) desconectNode);
}

int connections_node_getNodeSocket(char *nodeId) {
	// TODO, mutex por nodo? o en el sendoperation.
	int *nodeSocket = dictionary_get(nodesSockets, nodeId);
	return (nodeSocket ? *nodeSocket : -1);
}

void connections_node_setNodeSocket(char *nodeId, int socket) {
	int *socketAcceptedPtr = malloc(sizeof(int));
	*socketAcceptedPtr = socket;
	dictionary_put(nodesSockets, nodeId, socketAcceptedPtr);
}

int connections_node_getConnectedCount() {
	// TODO mutex
	return dictionary_size(nodesSockets);
}

void connections_node_accept(int socketAccepted, char *clientIP) {

	void *buffer;
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet(socketAccepted, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return;
	}

	// NODE DESERIALZE ..
	uint8_t isNewNode;
	uint16_t blocksCount;
	uint16_t sName;
	char *nodeName;

	memcpy(&isNewNode, buffer, sizeof(isNewNode));
	memcpy(&blocksCount, buffer + sizeof(isNewNode), sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);
	memcpy(&sName, buffer + sizeof(isNewNode) + sizeof(blocksCount), sizeof(sName));
	sName = ntohs(sName);
	nodeName = malloc(sizeof(char) * (sName + 1));
	memcpy(nodeName, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(sName), sName);
	nodeName[sName] = '\0';
	free(buffer);
	// ...

	//  Save the socket as a reference to this node.
	// TODO, ver en que usar la ip.
	connections_node_setNodeSocket(nodeName, socketAccepted);

	filesystem_addNode(nodeName, blocksCount, (bool) isNewNode);

	free(nodeName);
}

bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation) {

	int nodeSocket = connections_node_getNodeSocket(sendOperation->node->id);
	if (nodeSocket == -1) {
		return 0;
	}

	uint8_t command = NODE_COMMAND_SET_BLOCK;
	uint16_t numBlock = sendOperation->blockIndex;
	uint32_t sBlockData = strlen(sendOperation->block);

	size_t sBuffer = sizeof(command) + sizeof(numBlock) + sizeof(sBlockData) + sBlockData;

	uint16_t numBlockSerialized = htons(numBlock);
	uint32_t sBlockDataSerialized = htonl(sBlockData);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));
	memcpy(buffer + sizeof(command) + sizeof(numBlock), &sBlockDataSerialized, sizeof(sBlockData));
	memcpy(buffer + sizeof(command) + sizeof(numBlock) + sizeof(sBlockData), sendOperation->block, sBlockData);

	e_socket_status status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	return (status == SOCKET_ERROR_NONE);
}

char* connections_node_getBlock(file_block_t *fileBlock) {

	int nodeSocket = connections_node_getNodeSocket(fileBlock->nodeId);
	if (nodeSocket == -1) {
		return NULL;
	}

	e_socket_status status;

	uint8_t command = NODE_COMMAND_GET_BLOCK;
	uint16_t numBlock = fileBlock->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);

	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	// Wait for the response..

	buffer = NULL;
	sBuffer = 0;
	status = socket_recv_packet(nodeSocket, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	char *block = malloc(sBuffer + 1);
	memcpy(block, buffer, sBuffer);
	block[sBuffer] = '\0';

	free(buffer);

	return block;
}
