#include "marta.h"

void startMarta() {
	char FS_IP[] = "127.0.0.1";
	int FS_PORT = 5000;

	printf("Cliente MARTA\n");
	int fsSocket = -1;
	while (0 > fsSocket) {
		fsSocket = socket_connect(FS_IP, FS_PORT);
	}

	int hand = socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA);
	if (!hand) {
		printf("Error al conectar con el FS\n");
		return;
	}

	printf("Me conecte con el fs :D\n");

	// TEST PEDIR BLOQUES.
	pedirBloquesArchivo(fsSocket);

	while(1) {
		//printf("KEEPS CONNECTED");
	}
	printf("STATUS CLOSE %d\n", socket_close(fsSocket));
}

void marta_escucharAcciones(int fsSocket) {
	while (1) {
		void *buffer;
		size_t sbuffer = 0;
		printf("Waiting for FS instructions.\n");
		socket_recv_packet(fsSocket, &buffer, &sbuffer); // TODO handlear el error

		//  DESERIALZE ..

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case 1:

			break;
		case 2:

			break;
		}

		free(buffer);
	}
}

void pedirBloquesArchivo(int fsSocket) {

	uint8_t command = COMMAND_MARTA_TO_FS_GET_FILE_BLOCKS; // Comando para pedir los bloques del archivo.
	char archivo[] = "a";
	uint32_t sArchivo = strlen(archivo);

	size_t sBuffer = sizeof(command) + sizeof(sArchivo) + sArchivo;

	uint32_t sArchivoSerialized = htonl(sArchivo);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &command, sizeof(command));
	memcpy(buffer + sizeof(command), &sArchivoSerialized, sizeof(sArchivo));
	memcpy(buffer + sizeof(command) + sizeof(sArchivo), &archivo, sArchivo);

	socket_send_packet(fsSocket, buffer, sBuffer); // TODO, chequear status.

	free(buffer);

	// Espero repuesta:

	size_t sbuffer = 0;
	e_socket_status status = socket_recv_packet(fsSocket, &buffer, &sbuffer); // TODO handlear el error

	if (status != SOCKET_ERROR_NONE) {
		// TODO, disconnected from server.
		return;
	}
	uint16_t blocksCount;
	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	void *bufferOffset = buffer + sizeof(blocksCount);

	printf("Bloques de %s . Tiene %d bloques \n", archivo, blocksCount);
	fflush(stdout);
	int fileBlockNumber;
	for (fileBlockNumber = 0; fileBlockNumber < blocksCount; fileBlockNumber++) {
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
			uint16_t nodeBlockNumber;

			memcpy(&sNodeId, bufferOffset, sizeof(sNodeId));
			sNodeId = ntohl(sNodeId);
			bufferOffset += sizeof(sNodeId);

			nodeId = malloc(sizeof(char) * (sNodeId + 1));
			memcpy(nodeId, bufferOffset, sNodeId);
			nodeId[sNodeId] = '\0';
			bufferOffset += sNodeId;

			memcpy(&nodeBlockNumber, bufferOffset, sizeof(nodeBlockNumber));
			nodeBlockNumber = ntohs(nodeBlockNumber);
			bufferOffset += sizeof(nodeBlockNumber);

			// TODO, aca guardar en una estructura, o usarlo, nose.
			printf("\t|\t|---->  Nodo: %s . Bloque nro: %d \n", nodeId, nodeBlockNumber);
			fflush(stdout);
			free(nodeId);
		}
	}
	printf("---------------------------------------------------------\n");
	fflush(stdout);

	free(buffer);
}
