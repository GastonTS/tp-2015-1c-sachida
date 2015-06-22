#include "connections_fs.h"
#include "connections.h"

void *connections_fs_connect(void *param);
void connections_fs_sendInfo(t_nodeCfg *config);
void connections_fs_listenActions();
void connections_fs_deserializeSetBlock(void *buffer);
void connections_fs_deserializeGetBlock(void *buffer);

int fsSocket;
int exitFs;

void connections_fs_initialize(t_nodeCfg *config) {
	exitFs = 0;
	fsSocket = -1;
	pthread_t fsTh;
	if (pthread_create(&fsTh, NULL, (void *) connections_fs_connect, (void *) config)) {
		log_error(node_logger, "Error while trying to create new thread: connections_fs_connect");
	}
	pthread_detach(fsTh);
}

void connections_fs_shutdown() {
	exitFs = 1;
}

void *connections_fs_connect(void *param) {
	t_nodeCfg *config = (t_nodeCfg *) param;

	while (!exitFs) {
		if (0 > fsSocket) {
			while (0 > fsSocket && !exitFs) {
				fsSocket = socket_connect(config->ip_fs, config->puerto_fs);
			}
			if (fsSocket >= 0) {
				if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
					socket_close(fsSocket);
					fsSocket = -1;
					log_error(node_logger, "Handshake to filesystem failed.");
				} else {
					log_info(node_logger, "Connected to FileSystem.");
					connections_fs_sendInfo(config);
				}
			}
		}
	}
	socket_close(fsSocket);
	fsSocket = -1;
	return NULL;
}

void connections_fs_sendInfo(t_nodeCfg *config) {
	// TODO from config.
	uint16_t cantBloques = 30;
	char myName[] = "Nodo1"; // Le paso mi nombre.

	uint16_t sName = strlen(myName);
	size_t sBuffer = sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(sName) + sName + sizeof(config->puerto_nodo);

	uint16_t cantBloquesSerialized = htons(cantBloques);
	uint16_t puertoNodoSerialized = htons(config->puerto_nodo);
	uint16_t sNameSerialized = htons(sName);

	void *pBuffer = malloc(sBuffer);
	memcpy(pBuffer, &config->nodo_nuevo, sizeof(config->nodo_nuevo));
	memcpy(pBuffer + sizeof(config->nodo_nuevo), &cantBloquesSerialized, sizeof(cantBloques));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques), &puertoNodoSerialized, sizeof(puertoNodoSerialized));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(puertoNodoSerialized), &sNameSerialized, sizeof(sNameSerialized));
	memcpy(pBuffer + sizeof(config->nodo_nuevo) + sizeof(cantBloques) + sizeof(puertoNodoSerialized) + sizeof(sName), &myName, sName);

	printf("va a mandar los datos  del nodo al fs\n");
	e_socket_status status = socket_send_packet(fsSocket, pBuffer, sBuffer);
	if (0 > status) {
		socket_close(fsSocket);
		fsSocket = -1;
	} else {
		connections_fs_listenActions();
	}

	free(pBuffer);
}

void connections_fs_listenActions() {
	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(fsSocket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(fsSocket);
			fsSocket = -1; // TODO mover a metodo y meter mutex por fsSocket.
			return;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case 1: //setBloque // TODO mover a socket.h
			printf("llego al deserialize set block\n");
			connections_fs_deserializeSetBlock(buffer);
			break;
		case 2: //getBloque
			printf("llego al deserialize get block\n");
			connections_fs_deserializeGetBlock(buffer);
			break;
		default:
			log_error(node_logger, "FS Sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_fs_deserializeSetBlock(void *buffer) {
	printf("llego al connections_fs_des SetBLock\n");
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);


	uint32_t sBlockData;
	memcpy(&sBlockData, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));


	char *blockData = malloc(sizeof(char) * (sBlockData + 1));
	memcpy(blockData, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sBlockData);
	blockData[sBlockData] = '\0';

	printf("3\n");
	node_setBlock(numBlock, blockData);

	free(blockData);
}

void connections_fs_deserializeGetBlock(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	char *blockData = node_getBlock(numBlock);

	if (blockData) {
		e_socket_status status = socket_send_packet(fsSocket, blockData, strlen(blockData));

		if (status != SOCKET_ERROR_NONE) {
			// TODO, manejar el error.
		}
	} else {
		// TODO handlear error.
	}

	node_freeBlock(blockData);
}

