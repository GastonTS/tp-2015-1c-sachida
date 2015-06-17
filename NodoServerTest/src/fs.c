#include "fs.h"

void startFS() {
	//char Node_IP[] = "127.0.0.1";
	t_port fsPORT = 5000;
	char *nodeIp;
	int nodeSocket;
	logger = log_create("Log.txt", "Servidor",1, log_level_from_string("DEBUG"));
	log_info(logger,"puerto: %d",fsPORT);
	log_info(logger,"Arranque");
	int fsSocket = socket_listen(fsPORT);
	nodeSocket = socket_accept_and_get_ip(fsSocket, &nodeIp);
	int hand = socket_handshake_to_client(nodeSocket,HANDSHAKE_FILESYSTEM,HANDSHAKE_NODO);
	if (!hand) {
		printf("Error al conectar con el Nodo\n");
		return;
	}

	log_info(logger,"Conection sucessfully");

	// Levanto el nodo
	log_info(logger,"levanto el nodo\n");
	size_t sBuffer;
	char *buffer;
	log_info(logger,"ahora trato de recibir\n");
	socket_recv_packet(nodeSocket, (void**) buffer, &sBuffer);
	log_info(logger,"recibo\n");
	free(buffer);

	// AHORA ESPERO ACCIONES DEL FS:
	// EL NODO DEBERIA TIRAR THREADS ACA
	nodo_escucharAcciones(nodeSocket);
}

void nodo_escucharAcciones(int fsSocket) {
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
			deserialzeGetBloque(buffer, fsSocket);
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
	fflush(stdout);
}

void deserialzeGetBloque(void *buffer, int fsSocket) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	char *bloque = getBloque(numBlock);

	e_socket_status status = socket_send_packet(fsSocket, bloque, strlen(bloque));

	if (status != SOCKET_ERROR_NONE) {
		// TODO, manejar el error.
	}

	free(bloque);
}

char* getBloque(uint16_t numBlock) {
	printf("----------GET BLOQUE-----------\nObtengo el bloque numero %d\n----------------------------\n", numBlock);
	fflush(stdout);
	return strdup("Este es el bloque");
}
