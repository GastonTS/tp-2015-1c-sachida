#include "connections_node.h"
#include "connections.h"
#include "../node.h"

void* connections_node_listenActions(void *param);

void connections_node_initialize() {

}

void connections_node_shutdown() {

}

char* connections_node_getFileContent(node_connection_getTmpFileOperation_t *operation) {
	int socket = -1;

	socket = socket_connect(operation->ip, operation->port);

	if (socket >= 0) {
		if (HANDSHAKE_NODO != socket_handshake_to_server(socket, HANDSHAKE_NODO, HANDSHAKE_NODO)) {
			log_error(node_logger, "Handshake to node failed.");
			socket_close(socket);
			return NULL;
		} else {
			// Request the tmp file content.
			e_socket_status status;

			uint8_t command = COMMAND_NODE_GET_TMP_FILE_CONTENT;
			uint32_t sTmpName = strlen(operation->tmpFileName);

			size_t sBuffer = sizeof(command) + sizeof(sTmpName) + sTmpName;
			uint32_t sTmpNameSerialized = htonl(sTmpName);

			void *buffer = malloc(sBuffer);
			memcpy(buffer, &command, sizeof(command));
			memcpy(buffer + sizeof(command), &sTmpNameSerialized, sizeof(sTmpName));
			memcpy(buffer + sizeof(command) + sizeof(sTmpName), operation->tmpFileName, sTmpName);

			status = socket_send_packet(socket, buffer, sBuffer);
			if (0 > status) {
				log_error(node_logger, "Connection to node failed.");
				socket_close(socket);
				return NULL;
			}
			free(buffer);

			buffer = NULL;
			status = socket_recv_packet(socket, &buffer, &sBuffer);
			if (0 > status) {
				log_error(node_logger, "Connection to node failed.");
				socket_close(socket);
				return NULL;
			}

			socket_close(socket);
			return buffer;
		}
	} else {
		log_error(node_logger, "Connection to node failed.");
		return NULL;
	}
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
	case COMMAND_NODE_GET_TMP_FILE_CONTENT:
		connections_deserializeGetFileContent(socket, buffer);
		break;
	default:
		log_error(node_logger, "NODE sent an invalid command %d", command);
		break;
	}
	free(buffer);

	socket_close(socket);
	return NULL;
}

node_connection_getTmpFileOperation_t* node_connection_getTmpFileOperation_create(char *nodeId, char *ip, uint16_t port, char *tmpFileName) {
	node_connection_getTmpFileOperation_t *operation = malloc(sizeof(node_connection_getTmpFileOperation_t));
	operation->nodeId = strdup(nodeId);
	operation->ip = strdup(ip);
	operation->port = port;
	operation->tmpFileName = strdup(tmpFileName);
	return operation;
}

void node_connection_getTmpFileOperation_free(node_connection_getTmpFileOperation_t *operation) {
	if (operation) {
		if (operation->nodeId) {
			free(operation->nodeId);
		}
		if (operation->ip) {
			free(operation->ip);
		}
		if (operation->tmpFileName) {
			free(operation->tmpFileName);
		}
		free(operation);
	}
}
