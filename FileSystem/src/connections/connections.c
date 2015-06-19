#include "connections.h"
#include "connections_marta.h"
#include "connections_node.h"

void *connections_listenerThread(void *param);

int exitConnections;
pthread_t listenerThread;


void connections_initialize(fs_connections_cfg_t *config) {
	connections_node_initialize();
	connections_marta_initialize();

	exitConnections = 0;

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) config)) {
		log_error(mdfs_logger, "Error while trying to create new thread: connections_listenerThread");
	}
}

void connections_shutdown() {
	connections_node_shutdown();
	connections_marta_shutdown();

	exitConnections = 1; // TODO TEMA CONNECTIONS ver bien como hacer esto..
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {
	fs_connections_cfg_t *config = (fs_connections_cfg_t *)param;
	int socketListener = socket_listen(config->port);
	int socketAccepted;

	while (!exitConnections) {
		char *clientIP;
		socketAccepted = socket_accept_and_get_ip(socketListener, &clientIP); // TODO TEMA CONNECTIONS se queda clavado acá el exit, como corto esto??

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA | HANDSHAKE_NODO)) {
		case HANDSHAKE_NODO:
			connections_node_accept(socketAccepted, clientIP); // TODO MOVER A THREAD
			break;
		case HANDSHAKE_MARTA:
			if (connections_node_getActiveConnectedCount() < config->minNodesCount) {
				log_info(mdfs_logger, "Marta connected but rejected because minimum nodes count was not reached\n");
				socket_close(socketAccepted);
			} else {
				connections_marta_accept(socketAccepted); // TODO MOVER A THREAD
			}
			break;
		}

		if (clientIP) {
			free(clientIP);
		}
	}

	socket_close(socketListener);
	return NULL;
}
