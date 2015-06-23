#include "connections.h"
#include "connections_marta.h"
#include "connections_node.h"

void *connections_listenerThread(void *param);

int socketListener;
pthread_t listenerThread;

void connections_initialize(fs_connections_cfg_t *config) {
	connections_node_initialize();
	connections_marta_initialize();

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) config)) {
		log_error(mdfs_logger, "Error while trying to create new thread: connections_listenerThread");
	}
}

void connections_shutdown() {
	connections_node_shutdown();
	connections_marta_shutdown();

	socket_close(socketListener);
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {
	fs_connections_cfg_t *config = (fs_connections_cfg_t *) param;
	socketListener = socket_listen(config->port);
	int socketAccepted;

	while (1) {
		char *clientIP = NULL;
		socketAccepted = socket_accept_and_get_ip(socketListener, &clientIP);

		if (0 > socketAccepted) { // Connections where closed.
			return NULL;
		}

		pthread_t acceptedConnectionTh;
		int handshake = socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA | HANDSHAKE_NODO);

		if (handshake == HANDSHAKE_NODO) {
			node_connection_t *nodeConnection = connections_node_connection_create(socketAccepted, clientIP);
			if (pthread_create(&acceptedConnectionTh, NULL, (void *) connections_node_accept, (void *) nodeConnection)) {
				connections_node_connection_free(nodeConnection);
				log_error(mdfs_logger, "Error while trying to create new thread: connections_node_accept");
			}
			pthread_detach(acceptedConnectionTh);
		} else if (handshake == HANDSHAKE_MARTA) {
			if (connections_node_getActiveConnectedCount() < config->minNodesCount) {
				log_info(mdfs_logger, "Marta connected but rejected because minimum nodes count was not reached\n");
				socket_close(socketAccepted);
			} else {
				if (pthread_create(&acceptedConnectionTh, NULL, (void *) connections_marta_accept, (void *) &socketAccepted)) {
					log_error(mdfs_logger, "Error while trying to create new thread: connections_marta_accept");
				}
				pthread_detach(acceptedConnectionTh);
			}
		}

		if (clientIP) {
			free(clientIP);
		}
	}

	return NULL;
}
