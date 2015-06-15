#include "marta.h"

void startMarta() {
	char FS_IP[] = "127.0.0.1";
	int FS_PORT = 5000;

	printf("Cliente MARTA\n");
	int fsSocket = socket_connect(FS_IP, FS_PORT);
	int hand = socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_MARTA);
	if (!hand) {
		printf("Error al conectar con el FS\n");
		return;
	}

	printf("Me conecte con el fs :D\n");

	// AHORA ESPERO ACCIONES DEL FS:
	// MARTA DEBERIA TIRAR THREADS ACA
	//marta_escucharAcciones(fsSocket);

	pedirBloquesArchivo(fsSocket);
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

	uint8_t command = 1; // Comando para pedir los bloques del archivo.
	char archivo[] = "aa";
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
	socket_recv_packet(fsSocket, &buffer, &sbuffer); // TODO handlear el error

	uint16_t blocksCount;
	memcpy(&blocksCount, buffer, sizeof(blocksCount));
	blocksCount = ntohs(blocksCount);

	void *bufferOffset = buffer + sizeof(blocksCount);

	printf("Bloques de %s . Tiene %d bloques \n", archivo, blocksCount);
	int i;
	for (i = 0; i < blocksCount; i++) {
		uint16_t copyesCount;
		memcpy(&copyesCount, bufferOffset, sizeof(copyesCount));
		copyesCount = ntohs(copyesCount);
		bufferOffset += sizeof(copyesCount);

		printf("\tBloque nro %d. Tiene %d copias \n", i, copyesCount);
		int j;
		for (j = 0; j < copyesCount; j++) {
			uint32_t sNodeId;
			char *nodeId;
			uint16_t blockNumber;

			memcpy(&sNodeId, bufferOffset, sizeof(sNodeId));
			sNodeId = ntohl(sNodeId);
			bufferOffset += sizeof(sNodeId);

			nodeId = malloc(sizeof(char) * (sNodeId + 1));
			memcpy(nodeId, bufferOffset, sNodeId);
			nodeId[sNodeId] = '\0';
			bufferOffset += sNodeId;

			memcpy(&blockNumber, bufferOffset, sizeof(blockNumber));
			blockNumber = ntohs(blockNumber);
			bufferOffset += sizeof(blockNumber);

			// TODO, aca guardar en una estructura, o usarlo, nose.
			printf("\t\t Nodo: %s . Bloque nro: %d \n", nodeId, blockNumber);
			free(nodeId);
		}
	}

	free(buffer);
}
