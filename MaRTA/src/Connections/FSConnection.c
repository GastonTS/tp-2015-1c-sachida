#include "Connection.h"
#include "FSConnection.h"

void connectFS();

int fsSocket;

void initFSConnection() {
	fsSocket = -1;
	connectFS();
}

void FSConnectionLost() {
	log_error(logger, "Connection to FS was lost.");
	socket_close(fsSocket);
	fsSocket = -1;
	connectFS();
}

void connectFS() {
	if (0 > fsSocket) {
		log_info(logger, "Connecting MDFS...");
		while (0 > fsSocket) {
			fsSocket = socket_connect(cfgMaRTA->fsIP, cfgMaRTA->fsPort);
		}
		if (fsSocket >= 0) {
			if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
				log_error(logger, "Handshake to filesystem failed.");
				FSConnectionLost();
			} else {
				log_info(logger, "Connected to FileSystem.");
			}
		}
	}
}

void requestFileBlocks(t_file *file) {
	void *buffer;
	void request() {
		uint8_t command = COMMAND_MARTA_TO_FS_GET_FILE_BLOCKS;
		uint32_t sfilePath = strlen(file->path);

		size_t sBuffer = sizeof(command) + sizeof(sfilePath) + sfilePath;

		uint32_t sArchivoSerialized = htonl(sfilePath);

		buffer = malloc(sBuffer);
		memcpy(buffer, &command, sizeof(command));
		memcpy(buffer + sizeof(command), &sArchivoSerialized, sizeof(sfilePath));
		memcpy(buffer + sizeof(command) + sizeof(sfilePath), &file->path, sfilePath);

		while (0 > socket_send_packet(fsSocket, buffer, sBuffer)) {
			FSConnectionLost();
		}
		free(buffer);
	}
	request();

	size_t sbuffer = 0;
	while (0 > socket_recv_packet(fsSocket, &buffer, &sbuffer)) {
		FSConnectionLost();
		request();
	}

	uint16_t blocksCount;
	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	if (blocksCount == 0) {
		// TODO handlear caso el archivo no existe, o no esta disponible por falta de nodos, etc.
		printf("ARCHIVO NO DISPONIBLE.......... jejejeje");
		fflush(stdout);
		free(buffer);
		return;
	}
	void *bufferOffset = buffer + sizeof(blocksCount);

	printf("\nBloques de %s . Tiene %d \n ", file->path, blocksCount);
	fflush(stdout);
	int fileBlockNumber;
	for (fileBlockNumber = 0; fileBlockNumber < blocksCount; fileBlockNumber++) {
		t_list *copies = list_create();
		uint16_t copyesCount;
		memcpy(&copyesCount, bufferOffset, sizeof(copyesCount));
		copyesCount = ntohs(copyesCount);
		bufferOffset += sizeof(copyesCount);

		printf("\t|----> Bloque nro %d. Tiene %d copias \n", fileBlockNumber, copyesCount);
		fflush(stdout);
		int j;
		for (j = 0; j < copyesCount; j++) {
			uint32_t sNodeId;
			char *nodeId;
			uint16_t sNodeIp;
			char *nodeIp;
			uint16_t nodePort;
			uint16_t nodeBlockNumber;

			memcpy(&sNodeId, bufferOffset, sizeof(sNodeId));
			sNodeId = ntohl(sNodeId);
			bufferOffset += sizeof(sNodeId);

			nodeId = malloc(sizeof(char) * (sNodeId + 1));
			memcpy(nodeId, bufferOffset, sNodeId);
			nodeId[sNodeId] = '\0';
			bufferOffset += sNodeId;

			memcpy(&sNodeIp, bufferOffset, sizeof(sNodeIp));
			sNodeIp = ntohs(sNodeIp);
			bufferOffset += sizeof(sNodeIp);

			nodeIp = malloc(sizeof(char) * (sNodeIp + 1));
			memcpy(nodeIp, bufferOffset, sNodeIp);
			nodeIp[sNodeIp] = '\0';
			bufferOffset += sNodeIp;

			memcpy(&nodePort, bufferOffset, sizeof(nodePort));
			nodePort = ntohs(nodePort);
			bufferOffset += sizeof(nodePort);

			memcpy(&nodeBlockNumber, bufferOffset, sizeof(nodeBlockNumber));
			nodeBlockNumber = ntohs(nodeBlockNumber);
			bufferOffset += sizeof(nodeBlockNumber);

			//TODO: segundo y tercer parametro de CreateCopy, IP y Puerto respectivamente.
			t_copy *copy = CreateCopy(nodeId, nodeIp, nodePort, nodeBlockNumber);
			list_add(copies, copy);
			printf("\t|\t|---->  Nodo: %s . Bloque nro: %d \n", nodeId, nodeBlockNumber);
			fflush(stdout);
			free(nodeId);
			free(nodeIp);
		}
		list_add(file->blocks, copies);
	}
	free(buffer);
}

