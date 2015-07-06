#include "connections_node.h"
#include "connections.h"
#include "../node.h"

void* connections_node_listenActions(void *param);
void connections_node_deserializeGetFileContent(int socket, void *buffer);

void connections_node_initialize() {

}

void connections_node_shutdown() {

}

char* connections_node_getFileContent(char *ip, uint16_t port, char *tmpFileName) {
	int socket = -1;

	socket = socket_connect(ip, port);

	printf("socket %d", socket);
	if (socket >= 0) {
		if (HANDSHAKE_NODO != socket_handshake_to_server(socket, HANDSHAKE_NODO, HANDSHAKE_NODO)) {
			log_error(node_logger, "Handshake to node failed.");
			socket_close(socket);
			return NULL;
		} else {
			// Request the tmp file content.
			e_socket_status status;

			uint8_t command = COMMAND_NODE_GET_TMP_FILE_CONTENT;
			uint32_t sTmpName = strlen(tmpFileName);

			size_t sBuffer = sizeof(command) + sizeof(sTmpName) + sTmpName;
			uint32_t sTmpNameSerialized = htonl(sTmpName);

			void *buffer = malloc(sBuffer);
			memcpy(buffer, &command, sizeof(command));
			memcpy(buffer + sizeof(command), &sTmpNameSerialized, sizeof(sTmpName));
			memcpy(buffer + sizeof(command) + sizeof(sTmpName), tmpFileName, sTmpName);

			status = socket_send_packet(socket, buffer, sBuffer);
			if (0 > status) {
				log_error(node_logger, "Connection to node failed.");
				socket_close(socket);
				return NULL;
			}
			free(buffer);

			status = socket_recv_packet(socket, &buffer, &sBuffer);
			if (0 > status) {
				log_error(node_logger, "Connection to node failed.");
				socket_close(socket);
				return NULL;
			}

			char *tmpFileContent = realloc(buffer, sBuffer + 1);
			tmpFileContent[sBuffer] = '\0';

			socket_close(socket);
			return tmpFileContent;
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
		connections_node_deserializeGetFileContent(socket, buffer);
		break;
	default:
		log_error(node_logger, "NODE sent an invalid command %d", command);
		break;
	}
	free(buffer);

	socket_close(socket);
	return NULL;
}

void connections_node_deserializeGetFileContent(int socket, void *buffer) {
	size_t offset = sizeof(uint8_t);

	uint32_t sTmpName;
	memcpy(&sTmpName, buffer + offset, sizeof(sTmpName));
	sTmpName = ntohl(sTmpName);
	offset += sizeof(sTmpName);

	char* tmpName = malloc(sizeof(char) * (sTmpName + 1));
	memcpy(tmpName, buffer + offset, sTmpName);
	tmpName[sTmpName] = '\0';
	offset += sTmpName;

	log_info(node_logger, "Node Requested tmpFileContent of %s", tmpName);

	char *tmpFileContent = node_getTmpFileContent(tmpName);

	socket_send_packet(socket, tmpFileContent, strlen(tmpFileContent));

	free(tmpName);
	free(tmpFileContent);
}

