#include "connections_node.h"
#include "connections.h"
#include "../node.h"

void* connections_node_listenActions(void *param);

void connections_node_initialize() {

}

void connections_node_shutdown() {

}

void* connections_node_accept(void *param) {
	int *socketAcceptedPtr = (int *) param;

	log_info(node_logger, "New node connected.");

	pthread_t listenActionsTh;
	if (pthread_create(&listenActionsTh, NULL, (void *) connections_node_listenActions, (void *) socketAcceptedPtr)) {
		free(socketAcceptedPtr);
		log_error(node_logger, "Error while trying to create new thread: connections_node_listenActions");
	}
	pthread_detach(listenActionsTh);

	return NULL;
}

void* connections_node_listenActions(void *param) {
	int *socketAcceptedPtr = (int *) param;
	int socket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(socket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(socket);
			return NULL;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case COMMAND_MAP:

			break;
		default:
			log_error(node_logger, "NODE sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}
