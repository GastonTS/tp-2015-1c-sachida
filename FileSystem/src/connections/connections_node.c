#include "connections_node.h"
#include "connections.h"

#include <commons/collections/dictionary.h>

pthread_mutex_t activeNodesLock;
pthread_mutex_t standbyNodesLock;
pthread_mutex_t m;
t_dictionary *activeNodesSockets;
t_dictionary *standbyNodesSockets;

void connections_node_initialize() {
	pthread_mutex_init(&m, NULL);

	if (pthread_mutex_init(&activeNodesLock, NULL) != 0 || pthread_mutex_init(&standbyNodesLock, NULL) != 0) {
		log_error(mdfs_logger, "Error while trying to create new mutex");
		return;
	}
	activeNodesSockets = dictionary_create();
	standbyNodesSockets = dictionary_create();
}

void connections_node_shutdown() {
	// TODO lock desdtroy por nodo.
	pthread_mutex_destroy(&activeNodesLock);
	pthread_mutex_destroy(&standbyNodesLock);

	void disconnectNode(node_connection_t *nodeConnection) {
		socket_close(nodeConnection->socket);
		connections_node_connection_free(nodeConnection);
	}
	dictionary_destroy_and_destroy_elements(activeNodesSockets, (void*) disconnectNode);
	dictionary_destroy_and_destroy_elements(standbyNodesSockets, (void*) disconnectNode);
}

node_connection_t* connections_node_getNodeConnection(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_get(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	return nodeConnection;
}

void connections_node_setNodeConnection(char *nodeId, node_connection_t *nodeConnection) {
	pthread_mutex_lock(&standbyNodesLock);
	dictionary_put(standbyNodesSockets, nodeId, nodeConnection);
	pthread_mutex_unlock(&standbyNodesLock);
}

void connections_node_removeNodeConnection(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	connections_node_connection_free(nodeConnection);
}

void connections_node_activateNode(char *nodeId) {
	pthread_mutex_lock(&standbyNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(standbyNodesSockets, nodeId);
	pthread_mutex_unlock(&standbyNodesLock);

	if (nodeConnection) {
		pthread_mutex_lock(&activeNodesLock);
		dictionary_put(activeNodesSockets, nodeId, nodeConnection);
		pthread_mutex_unlock(&activeNodesLock);
	}
}

void connections_node_deactivateNode(char *nodeId) {
	pthread_mutex_lock(&activeNodesLock);
	node_connection_t *nodeConnection = dictionary_remove(activeNodesSockets, nodeId);
	pthread_mutex_unlock(&activeNodesLock);

	if (nodeConnection) {
		pthread_mutex_lock(&standbyNodesLock);
		dictionary_put(standbyNodesSockets, nodeId, nodeConnection);
		pthread_mutex_unlock(&standbyNodesLock);
	}
}

int connections_node_getActiveConnectedCount() {
	pthread_mutex_lock(&activeNodesLock);
	return dictionary_size(activeNodesSockets);
	pthread_mutex_unlock(&activeNodesLock);
}

bool connections_node_isActiveNode(char *nodeId) {
	return (bool) connections_node_getNodeConnection(nodeId);
}

void* connections_node_accept(void *param) {
	node_connection_t *nodeConnection = (node_connection_t *) param;

	void *buffer;
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet(nodeConnection->socket, &buffer, &sBuffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	// NODE DESERIALZE ..
	uint8_t isNewNode;
	uint16_t blocksCount;
	uint16_t listenPort;
	uint16_t sName;
	char *nodeName;

	memcpy(&isNewNode, buffer, sizeof(isNewNode));

	memcpy(&blocksCount, buffer + sizeof(isNewNode), sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	memcpy(&listenPort, buffer + sizeof(isNewNode) + sizeof(blocksCount), sizeof(listenPort));
	listenPort = ntohs(listenPort);

	memcpy(&sName, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort), sizeof(sName));
	sName = ntohs(sName);
	nodeName = malloc(sizeof(char) * (sName + 1));
	memcpy(nodeName, buffer + sizeof(isNewNode) + sizeof(blocksCount) + sizeof(listenPort) + sizeof(sName), sName);
	nodeName[sName] = '\0';

	free(buffer);
	// ...

	//  Save the connection as a reference to this node.
	nodeConnection->listenPort = listenPort;
	printf("IP NODO : %s\n", nodeConnection->ip);

	connections_node_setNodeConnection(nodeName, nodeConnection);

	log_info(mdfs_logger, "Node connected. Name: %s. listenPort %d. blocksCount %d. New: %s", nodeName, listenPort, blocksCount, isNewNode ? "true" : "false");
	filesystem_addNode(nodeName, blocksCount, (bool) isNewNode);

	free(nodeName);

	return NULL;
}

bool connections_node_sendBlock(nodeBlockSendOperation_t *sendOperation) {
	// TODO, deberia hacer un mutex por socket. SI, hasta el recv e que salio todo ok, sino es un bardo..

	pthread_mutex_lock(&m);

	log_info(mdfs_logger, "Going to SET block %d to node %s.", sendOperation->blockIndex, sendOperation->node->id);

	node_connection_t *nodeConnection = connections_node_getNodeConnection(sendOperation->node->id);
	if (!nodeConnection) {
		pthread_mutex_unlock(&m);
		return 0;
	}

	uint8_t command = COMMAND_FS_TO_NODE_SET_BLOCK;
	uint16_t numBlock = sendOperation->blockIndex;
	uint32_t sBlockData = strlen(sendOperation->block);

	size_t sBuffer = sizeof(command) + sizeof(numBlock) + sizeof(sBlockData) + sBlockData;

	uint16_t numBlockSerialized = htons(numBlock);
	uint32_t sBlockDataSerialized = htonl(sBlockData);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));
	memcpy(buffer + sizeof(command) + sizeof(numBlock), &sBlockDataSerialized, sizeof(sBlockData));
	memcpy(buffer + sizeof(command) + sizeof(numBlock) + sizeof(sBlockData), sendOperation->block, sBlockData);

	e_socket_status status = socket_send_packet(nodeConnection->socket, buffer, sBuffer);

	free(buffer);

	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", sendOperation->node->id);
		connections_node_removeNodeConnection(sendOperation->node->id);
		pthread_mutex_unlock(&m);
		return 0;
	}

	pthread_mutex_unlock(&m);
	// TODO, hacer un recv y esperar espuesta OK (hacer que el nodo mande. )
	return (status == SOCKET_ERROR_NONE);
}

char* connections_node_getBlock(file_block_t *fileBlock) {
	log_info(mdfs_logger, "Going to GET block %d from node %s.", fileBlock->blockIndex, fileBlock->nodeId);

	node_connection_t *nodeConnection = connections_node_getNodeConnection(fileBlock->nodeId);
	if (!nodeConnection) {
		return NULL;
	}

	uint8_t command = COMMAND_FS_TO_NODE_GET_BLOCK;
	uint16_t numBlock = fileBlock->blockIndex;

	size_t sBuffer = sizeof(command) + sizeof(numBlock);

	uint16_t numBlockSerialized = htons(numBlock);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &numBlockSerialized, sizeof(numBlock));

	e_socket_status status = socket_send_packet(nodeConnection->socket, buffer, sBuffer);

	free(buffer);

	if (status != SOCKET_ERROR_NONE) {
		return NULL;
	}

	// Wait for the response..

	buffer = NULL;
	sBuffer = 0;
	status = socket_recv_packet(nodeConnection->socket, &buffer, &sBuffer);

	if (0 > status) {
		log_info(mdfs_logger, "Removing node %s because it was disconnected", fileBlock->nodeId);
		connections_node_removeNodeConnection(fileBlock->nodeId);
		return NULL;
	}

	char *block = malloc(sBuffer + 1);
	memcpy(block, buffer, sBuffer);
	block[sBuffer] = '\0';

	free(buffer);

	return block;
}

node_connection_t* connections_node_connection_create(int socket, char *ip) {
	node_connection_t *nodeConnection = malloc(sizeof(node_connection_t));
	nodeConnection->ip = strdup(ip);
	nodeConnection->socket = socket;
	return nodeConnection;
}

void connections_node_connection_free(node_connection_t *nodeConnection) {
	if (nodeConnection) {
		if (nodeConnection->ip) {
			free(nodeConnection->ip);
		}
		free(nodeConnection);
	}
}
