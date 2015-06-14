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

void connections_initialize(int port) {
	testV = 0;
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
			connections_test(); // TODO delete
			// exitConnections = 1;

			break;
		}
	}

	// TODO.
	printf("SALI?????");
	fflush(stdout);
	//..

	socket_close(socketListener);
	return NULL;
}

void connections_acceptNode(int socketAccepted) {

	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(socketAccepted, &buffer, &sbuffer);

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

	//
	// TODO en este mismo thread funciona, pero en un nuevo thread da success, pero no se recibe del otro lado.. ver bien!
	connections_test();

	pthread_t t;
	void *test(void *param) {
		connections_test();
		return NULL;
	}
	if (pthread_create(&t, NULL, (void *) test, NULL)) {
		return; // -1; // TODO handle error
	}
	pthread_join(t, NULL);

	while (0) {
		if (testV) {

			printf("TESTV\n");
			fflush(stdout);
			connections_test();
			testV = 0;
		}
	}
	//

	free(nodeName);
}

void connections_test() {

	nodeBlockSendOperation_t *nodeSendBlockOperation = malloc(sizeof(nodeBlockSendOperation_t));
	node_t *node = node_create(10);
	node->id = strdup("Nodo1");
	nodeSendBlockOperation->node = node;
	nodeSendBlockOperation->blockIndex = 45876;
	nodeSendBlockOperation->block = strdup("HOLA1");

	connections_getBlockFromNode(nodeSendBlockOperation);
}

bool connections_sendBlockToNode(nodeBlockSendOperation_t *sendOperation) {

	int nodeSocket = connections_getNodeSocket(sendOperation->node->id);
	if (nodeSocket == -1) {
		// TODO que onda?
		printf("EL NODO NO ESTA CONECTADO ? :O\n");
		fflush(stdout);
		return 0;
	}

	uint8_t command = 1; // declare in header..
	uint16_t numBlock = sendOperation->blockIndex;
	uint32_t sBlockData = strlen(sendOperation->block);

	size_t sBuffer = sizeof(command) + sizeof(numBlock) + sizeof(sBlockData) + sBlockData;

	uint16_t numBlockSerialized = htons(numBlock);
	uint32_t sBlockDataSerialized = htonl(sBlockData);

	void *buffer = malloc(sBuffer);
	memset(buffer, '\0', sBuffer); // TODO,borrar?
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));
	memcpy(buffer + sizeof(command) + sizeof(numBlock), &sBlockDataSerialized, sizeof(sBlockData));
	memcpy(buffer + sizeof(command) + sizeof(numBlock) + sizeof(sBlockData), sendOperation->block, sBlockData);

	e_socket_status status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	printf("SENT SET\n"); // Todo..
	fflush(stdout);

	return (status == SOCKET_ERROR_NONE);
}

char* connections_getBlockFromNode(nodeBlockSendOperation_t *sendOperation) {

	int nodeSocket = connections_getNodeSocket(sendOperation->node->id);
	if (nodeSocket == -1) {
		// TODO que onda?
		printf("EL NODO NO ESTA CONECTADO ? :O\n");
		fflush(stdout);
		return 0;
	}

	uint8_t command = 2; // declare in header..
	uint16_t numBlock = sendOperation->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);

	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memset(buffer, '\0', sBuffer); // TODO,borrar?
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	e_socket_status status = socket_send_packet(nodeSocket, buffer, sBuffer);

	free(buffer);

	printf("SENT GET\n"); // Todo..
	fflush(stdout);

	return ""; // TODO .
}
