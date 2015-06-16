#include "connections.h"
#include "connections_marta.h"
#include "connections_node.h"

void *connections_listenerThread(void *param);

int exitConnections;
pthread_t listenerThread;

// TODO. crear un logger de connections? SI, con func y semaphore.
t_log* connections_logger;

void connections_initialize(int port) {
	connections_logger = log_create("filesystem.log", "MDFS", 0, log_level_from_string("TRACE"));

	connections_node_initialize();
	connections_marta_initialize();

	exitConnections = 0;

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) &port)) {
		return; // -1; // TODO handle error
	}
}

void connections_shutdown() {
	connections_node_shutdown();
	connections_marta_shutdown();

	log_destroy(connections_logger);

	exitConnections = 1; // TODO , ver bien como hacer esto..
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {
	int port = *((int *) param);
	int socketListener = socket_listen(port);
	int socketAccepted;

	while (!exitConnections) {
		char *clientIP;
		socketAccepted = socket_accept_and_get_ip(socketListener, &clientIP); // TODO. se queda clavado acá el exit, como corto esto??

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA | HANDSHAKE_NODO)) {
		case HANDSHAKE_NODO:
			connections_node_accept(socketAccepted, clientIP); // TODO, mover a thread? SI. hacer.
			break;
		case HANDSHAKE_MARTA:
			connections_marta_accept(socketAccepted); // TODO, mover a thread? SI. hacer.
			break;
		}

		if (clientIP) {
			free(clientIP);
		}
	}

	socket_close(socketListener);
	return NULL;
}
