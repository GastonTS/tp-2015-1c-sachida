#include "connections.h"
#include "connections_fs.h"
#include "connections_job.h"

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
	connections_fs_shutdown();
	connections_job_shutdown();

	socket_close(socketListener);
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {

	socketListener = socket_listen(node_config->listenPort);
	int socketAccepted;

	while (1) {
		socketAccepted = socket_accept(socketListener);

		if (0 > socketAccepted) { // Connections where closed.
			return NULL;
		}

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_JOB)) {
		case HANDSHAKE_JOB:
			connections_job_accept(socketAccepted); // TODO MOVER A THREAD (sure?)
			break;
		}

	}

	return NULL;
}
