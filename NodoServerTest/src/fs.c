#include "fs.h"

void startFS() {
	//char Node_IP[] = "127.0.0.1";
	t_port fsPORT = 5000;
	char *nodeIp;
	int nodeSocket;
	logger = log_create("Log.txt", "Servidor",1, log_level_from_string("DEBUG"));
	log_info(logger,"File System port: %d",fsPORT);
	log_info(logger,"Listening connections");
	int fsSocket = socket_listen(fsPORT);
	nodeSocket = socket_accept_and_get_ip(fsSocket, &nodeIp);
	int hand = socket_handshake_to_client(nodeSocket,HANDSHAKE_FILESYSTEM,HANDSHAKE_NODO);
	if (!hand) {
		printf("Error al conectar con el Nodo\n");
		return;
	}

	log_info(logger,"Conection sucessfully");

	// Levanto el nodo
	size_t sBuffer;
	void *buffer;
	log_info(logger,"Recv packet");
	socket_recv_packet(nodeSocket, &buffer, &sBuffer);
	log_info(logger,"Recv packet succesfully");
	free(buffer);

	fs_enviarAcciones(nodeSocket);
}

void fs_enviarAcciones(int nodeSocket) {
		// op 1, bloque 21, leng 0002, bloque FF
		char paqueteS[] = { 0b00000001, 0b00010101, 0b00000000, 0b00000010,
				0b00000000, 0b00000000, 0b00000000, 0b01000110,0b01000110};
		size_t sbufferS = sizeof(paqueteS);
		char *bufferS = malloc(sbufferS) ;
		memcpy(bufferS,paqueteS,sbufferS );
		socket_send_packet(nodeSocket, bufferS, sbufferS);
		printf("Send OK\n");// TODO handlear el error
		free(bufferS);
		// op 1, bloque 21, leng 0002, bloque FF
		char paqueteG[] = { 0b00000010, 0b00010101, 0b00000000, 0b00000010,
				0b00000000, 0b00000000, 0b00000000, 0b01000110,0b01000110};
		size_t sbufferG = sizeof(paqueteG);
		char *bufferG = malloc(sbufferG) ;
		memcpy(bufferG,paqueteG,sbufferG );
		socket_send_packet(nodeSocket, bufferG, sbufferG);
		printf("Send OK\n");// TODO handlear el error
		free(bufferG);
}
