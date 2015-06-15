#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utils/socket.h"

void escucharAcciones(int fsSocket);

void deserialzeSetBloque(void *buffer);
void setBloque(uint16_t numBlock, char *blockData);

void deserialzeGetBloque(void *buffer);
void getBloque(uint16_t numBlock);

int main(void) {
	char FS_IP[] = "127.0.0.1";
	int FS_PORT = 5000;

	int fsSocket = socket_connect(FS_IP, FS_PORT);
	int hand = socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO);
	if (!hand) {
		printf("Error al conectar con el FS\n");
		return EXIT_FAILURE;
	}

	printf("Me conecte con el fs :D\n");

	// Le mando mi info para que me levante como nodo.

	uint16_t cantBloques = 10; // Le voy a decir que tengo 10 bloques para usar.
	char myName[] = "Nodo1"; // Le paso mi nombre.

	uint16_t sName = strlen(myName);
	size_t sBuffer = sizeof(cantBloques) + sizeof(sName) + sName;

	uint16_t cantBloquesSerialized = htons(cantBloques);
	uint16_t sNameSerialized = htons(sName);

	void *buffer = malloc(sBuffer);
	memcpy(buffer, &cantBloquesSerialized, sizeof(cantBloques));
	memcpy(buffer + sizeof(cantBloques), &sNameSerialized, sizeof(sName));
	memcpy(buffer + sizeof(cantBloques) + sizeof(sName), &myName, sName);

	socket_send_packet(fsSocket, buffer, sBuffer);

	free(buffer);

	// AHORA ESPERO ACCIONES DEL FS:
	// EL NODO DEBERIA TIRAR THREADS ACA
	escucharAcciones(fsSocket);

	return EXIT_SUCCESS;
}

void escucharAcciones(int fsSocket) {
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
			// setBloque
			deserialzeSetBloque(buffer);
			break;
		case 2:
			//getBloque
			deserialzeGetBloque(buffer);
			break;
		}

		free(buffer);
	}
}

void deserialzeSetBloque(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sBlockData;
	memcpy(&sBlockData, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sBlockData = ntohl(sBlockData);

	char* blockData = malloc(sizeof(char) * (sBlockData + 1));
	memcpy(blockData, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sBlockData);
	blockData[sBlockData] = '\0';

	setBloque(numBlock, blockData);

	free(blockData);
}

void setBloque(uint16_t numBlock, char *blockData) {
	printf("----------SET BLOQUE-----------\nSeteo en el bloque numero %d . DATA: \n%s\n----------------------------\n", numBlock, blockData);
}

void deserialzeGetBloque(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	getBloque(numBlock);
}

void getBloque(uint16_t numBlock) {
	printf("----------GET BLOQUE-----------\nObtengo el bloque numero %d\n----------------------------\n", numBlock);
}
