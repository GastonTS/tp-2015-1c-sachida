#include "connections.h"
#include "connections_fs.h"
#include "connections_job.h"

void *connections_listenerThread(void *param);

int socketListener;
pthread_t listenerThread;

void connections_initialize(t_nodeCfg *config) {
	connections_fs_initialize(config);
	// TODO connections_job_initialize();

	if (pthread_create(&listenerThread, NULL, (void *) connections_listenerThread, (void *) config)) {
		log_error(node_logger, "Error while trying to create new thread: connections_listenerThread");
	}
}

void connections_shutdown() {
	connections_fs_shutdown();
	// TODO connections_job_shutdown();

	socket_close(socketListener);
	pthread_join(listenerThread, NULL);
}

void *connections_listenerThread(void *param) {
	t_nodeCfg *config = (t_nodeCfg *) param;
	socketListener = socket_listen(config->puerto_nodo);
	int socketAccepted;

	while (1) {
		socketAccepted = socket_accept(socketListener);

		if (0 > socketAccepted) { // Connections where closed.
			return NULL;
		}

		switch (socket_handshake_to_client(socketAccepted, HANDSHAKE_FILESYSTEM, HANDSHAKE_JOB)) {
		case HANDSHAKE_JOB:
			//connections_job_accept(socketAccepted); // TODO MOVER A THREAD
			break;
		}

	}

	return NULL;
}
