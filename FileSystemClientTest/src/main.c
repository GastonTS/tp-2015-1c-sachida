#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "utils/socket.h"

int fsSocket;

int main(void) {
	char FS_IP[] = "127.0.0.1";
	int FS_PORT = 5000;

	fsSocket = socket_connect(FS_IP, FS_PORT);
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

	return EXIT_SUCCESS;
}
