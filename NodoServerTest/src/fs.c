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
	log_info(logger,"levanto el nodo");
	size_t sBuffer;
	void *buffer;
	log_info(logger,"ahora trato de recibir");
	socket_recv_packet(nodeSocket, &buffer, &sBuffer);
	log_info(logger,"recibo");
	free(buffer);

	fs_enviarAcciones(nodeSocket);
}

uint8_t obtenerComando(char* paquete) {
	uint8_t command;
	memcpy(&command, paquete, sizeof(uint8_t));
	return command;
}

uint16_t obtenerNumBlock(char* paquete) {
	uint16_t numBlock;
	memcpy(&numBlock, paquete + sizeof(uint8_t), sizeof(uint16_t));
	return numBlock;
}

uint32_t obtenerSize(char* paquete) {
	uint32_t size;
	memcpy(&size, paquete + sizeof(uint8_t) + sizeof(uint16_t),
			sizeof(uint32_t));
	return size;
}

char* obtenerDatosBloque(char* paquete, uint32_t size) {
	char* packet = malloc(sizeof(char) * (size + 1));
	memcpy(packet,
			paquete + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t),
			size);
	packet[size] = '\0';
	return packet;
}



void fs_enviarAcciones(int nodeSocket) {
		// op 1, bloque 21, leng 0002, bloque FF
		char paquete[] = { 0b00000010, 0b00010101, 0b00000000, 0b00000010,
				0b00000000, 0b00000000, 0b00000000, 0b01000110,0b01000110};
		size_t sbuffer = sizeof(paquete);
		char *buffer = malloc(sbuffer) ;
		memcpy(buffer,paquete,sbuffer );
		uint32_t size = obtenerSize(paquete);
		socket_send_packet(nodeSocket, buffer, sbuffer); // TODO handlear el error
		printf("Se enviaron %d bytes de %d al Nodo.\n", sizeof(buffer), sbuffer);
		//  DESERIALZE ..
		/*
		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case 1:
			// setBloque
			deserialzeSetBloque(buffer);
			break;
		case 2:
			//getBloque
			deserialzeGetBloque(buffer, nodeSocket);
			break;
		}*/

		free(buffer);
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
