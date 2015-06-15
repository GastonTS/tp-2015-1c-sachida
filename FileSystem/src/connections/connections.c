#include "connections.h"

#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
// #include <semaphore.h>
#include <commons/collections/dictionary.h>
#include "../utils/socket.h"

void *connections_listenerThread(void *param);
void connections_acceptNode(int socketAccepted);

t_dictionary *nodesSockets;
int exitConnections;
pthread_t listenerThread;

// TODO. crear un logger de connections?

void connections_initialize(int port) {
	exitConnections = 0;
	nodesSockets = dictionary_create();

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) &port)) {
		return; // -1; // TODO handle error
	}
}

void connections_shutdown() {
	dictionary_destroy(nodesSockets);

	exitConnections = 1;
	pthread_join(listenerThread, NULL);
}

int connections_getNodeSocket(char *nodeId) {
	// TODO, mutex por nodo? o en el sendoperation.

	int *nodeSocket = dictionary_get(nodesSockets, nodeId);
	return (nodeSocket ? *nodeSocket : -1);
}

void connections_setNodeSocket(char *nodeId, int socket) {
	int *socketAcceptedPtr = malloc(sizeof(int));
	*socketAcceptedPtr = socket;
	dictionary_put(nodesSockets, nodeId, socketAcceptedPtr);
}

void *connections_listenerThread(void *param) {
	int port = *((int *) param);
	int socketListener = socket_listen(port);
	int socketAccepted;

	while (!exitConnections) {
		socketAccepted = socket_accept(socketListener); // TODO. se queda clavado acá el exit, como corto esto??

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {

		case HANDSHAKE_NODO:
			// Tengo que recibir sus datos entonces..
			connections_acceptNode(socketAccepted); // TODO, mover a thread? SI. hacer.
			break;
		}
	}

	socket_close(socketListener);
	return NULL;
}

void connections_acceptNode(int socketAccepted) {

	void *buffer;
	size_t sbuffer = 0;
	e_socket_status status = socket_recv_packet(socketAccepted, &buffer, &sbuffer);

	if (status != SOCKET_ERROR_NONE) {
		return;
	}

	// NODE DESERIALZE ..

	uint16_t blocksCount;
	uint16_t sName;
	char *nodeName;

	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);
	memcpy(&sName, buffer + sizeof(blocksCount), sizeof(sName));
	sName = ntohs(sName);
	nodeName = malloc(sizeof(char) * (sName + 1));
	memcpy(nodeName, buffer + sizeof(blocksCount) + sizeof(sName), sName);
	nodeName[sName] = '\0';
	free(buffer);
	// ...

	//  Save the socket as a reference to this node.
	connections_setNodeSocket(nodeName, socketAccepted);

	filesystem_addNode(nodeName, blocksCount);

	free(nodeName);
}

bool connections_sendBlockToNode(nodeBlockSendOperation_t *sendOperation) {

	int nodeSocket = connections_getNodeSocket(sendOperation->node->id);
	if (nodeSocket == -1) {
		return 0;
	}

	uint8_t command = 1; // declare in header..
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

char* connections_getBlockFromNode(file_block_t *fileBlock) {

	int nodeSocket = connections_getNodeSocket(fileBlock->nodeId);
	if (nodeSocket == -1) {
		return NULL;
	}

	e_socket_status status;

	uint8_t command = 2; // declare in header..
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
