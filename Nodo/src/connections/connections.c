#include "connections.h"
#include "connections_fs.h"
#include "connections_job.h"
#include "connections_node.h"

void *connections_listenerThread(void *param);

int socketListener;
pthread_t listenerThread;

void connections_initialize() {
	connections_fs_initialize();
	connections_job_initialize();

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, NULL)) {
		log_error(node_logger, "Error while trying to create new thread: connections_listenerThread");
	}
}

void connections_shutdown() {
	socket_close(socketListener);
	connections_fs_shutdown();
	connections_job_shutdown();

	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {

	socketListener = socket_listen(node_config->listenPort); // TODO testear address already used.
	int socketAccepted;

	while (1) {
		socketAccepted = socket_accept(socketListener);

		if (0 > socketAccepted) { // Connections where closed.
			return NULL;
		}

		pthread_t acceptedConnectionTh;
		int handshake = socket_handshake_to_client(socketAccepted, HANDSHAKE_NODO, HANDSHAKE_JOB | HANDSHAKE_NODO);

		if (handshake == HANDSHAKE_JOB) {
			int *socketAcceptedPtr = malloc(sizeof(socketAccepted));
			*socketAcceptedPtr = socketAccepted;
			if (pthread_create(&acceptedConnectionTh, NULL, (void *) connections_job_accept, (void *) socketAcceptedPtr)) {
				free(socketAcceptedPtr);
				log_error(node_logger, "Error while trying to create new thread: connections_job_accept");
			}
			pthread_detach(acceptedConnectionTh);
		} else if (handshake == HANDSHAKE_NODO) {
			int *socketAcceptedPtr = malloc(sizeof(socketAccepted));
			*socketAcceptedPtr = socketAccepted;
			if (pthread_create(&acceptedConnectionTh, NULL, (void *) connections_node_accept, (void *) socketAcceptedPtr)) {
				free(socketAcceptedPtr);
				log_error(node_logger, "Error while trying to create new thread: connections_node_accept");
			}
			pthread_detach(acceptedConnectionTh);
		}
	}

	return NULL;
}
