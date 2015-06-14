#include "connections.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
// #include <semaphore.h>
#include "../utils/socket.h"
#include "../structs/node.h"

void *connections_listenerThread(void *param);
void connections_acceptNode(int socketAccepted);
node_t* connections_deserializeNode(void *buffer);

int exitConnections;
pthread_t listenerThread;

void connections_initialize(int port) {
	exitConnections = 0;

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) &port)) {
		return; // -1; // TODO handle error
	}
}

void connections_shutdown() {
	exitConnections = 1;
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {
	int port = *((int *)param);
	int socketListener = socket_listen(port);
	int socketAccepted;

	while (!exitConnections) {
		printf("Waiting conecttion...\n"); // TODO logger.
		socketAccepted = socket_accept(socketListener); // TODO. se queda clavado acá el exit, como corto esto??

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {

		case HANDSHAKE_NODO:
			printf("\nSe conectó un nodo !!!!!\n");
			// Tengo que recibir sus datos entonces..
			connections_acceptNode(socketAccepted); // TODO, mover a thread?
			break;
		}
	}

	return NULL;
}

void connections_acceptNode(int socketAccepted) {

	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(socketAccepted, &buffer, &sbuffer);

	node_t *node = connections_deserializeNode(buffer);
	// TODO, ver que hacer con el nodo ?
	printf("Nuevo nodo recibido: %s . Tiene %d bloques.", node->id, node->blocksCount);
	node_free(node);
}

node_t* connections_deserializeNode(void *buffer) {
	uint16_t blocksCount;
	uint16_t sName;
	char *nodeName;

	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);
	memcpy(&sName, buffer + sizeof(blocksCount), sizeof(sName));
	sName = ntohs(sName);
	nodeName = malloc(sName + 1);
	memcpy(nodeName, buffer + sizeof(blocksCount) + sizeof(sName), sName);
	nodeName[sName] = '\0';
	free(buffer);

	node_t *node = node_create(blocksCount);
	node->id = nodeName;

	return node;
}
