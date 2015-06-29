#include "Connection.h"

void listenJobs();

int socketListener;

void initConnection() {
	initFSConnection();
	listenJobs();
}

void listenJobs() {

	socketListener = socket_listen(cfgMaRTA->listenPort);
	int socketAccepted;

	while (1) {
		log_info(logger, "Waiting Job...");
		socketAccepted = socket_accept(socketListener);

		if (0 > socketAccepted) {
			log_error(logger, "Accept Job Fail");
		}

		pthread_t acceptedJobTh;
		int handshake = socket_handshake_to_client(socketAccepted, HANDSHAKE_MARTA, HANDSHAKE_JOB);

		if (handshake == HANDSHAKE_JOB) {
			int *socketAcceptedPtr = malloc(sizeof(socketAccepted));
			*socketAcceptedPtr = socketAccepted;
			if (pthread_create(&acceptedJobTh, NULL, (void *) acceptJob, (void *) socketAcceptedPtr)) {
				log_error(logger, "Error while trying to create new thread: connections_job_accept");
			}
			pthread_detach(acceptedJobTh);
		}
	}
}
