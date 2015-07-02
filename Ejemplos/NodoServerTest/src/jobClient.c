#include "jobClient.h"

void startJob() {
	t_port nodoPORT = 5001;
	char *ip_Nodo = "127.0.0.1";
	int descriptorJob;
	logger = log_create("Log.txt", "Servidor", 1,
			log_level_from_string("DEBUG"));
	descriptorJob = socket_connect(ip_Nodo, nodoPORT);
	free(ip_Nodo);
	int hand = socket_handshake_to_server(descriptorJob,
	HANDSHAKE_NODO, HANDSHAKE_JOB);
	if (!hand) {
		printf("Error al conectar el job con el Nodo\n");
		return;
	}

	log_info(logger, "Job Conection sucessfully");

	// Levanto el nodo
	size_t sBuffer;
	void *buffer;
	log_info(logger, "Reconociendo el nodo");
	socket_recv_packet(descriptorJob, &buffer, &sBuffer);
	log_info(logger, "Se reconocio el nodo");
	free(buffer);
	job_enviarAcciones(descriptorJob);
	log_destroy(logger);
}

void job_enviarAcciones(int nodeSocket) {
	//uint8_t accion
	//uint16_t nro bloque
	//uint16_t size del archivo map
	//char * archivo map
	//uint16_t size del temproal
	//char * nombre del temporal
	// op 3, bloque 21, leng 0002, bloque FF

	char paqueteS[] = { 0b00000011, 0b00000101, 0b00000000, 0b00000001,
			0b00000000, 0b01000101, 0b00000000, 0b00000000, 0b01000100 };
	size_t sbufferS = sizeof(paqueteS);
	char *bufferS = malloc(sbufferS);
	memcpy(bufferS, paqueteS, sbufferS);
	socket_send_packet(nodeSocket, bufferS, sbufferS);
	printf("Send OK\n"); // TODO handlear el error
	free(bufferS);
	// op 4, bloque 21, leng 0002, bloque FF
	char paqueteG[] = { 0b00000011, 0b00000101, 0b00000000, 0b00000001,
			0b00000000, 0b01000101, 0b00000000, 0b00000000, 0b01000100 };
	size_t sbufferG = sizeof(paqueteG);
	char *bufferG = malloc(sbufferG);
	memcpy(bufferG, paqueteG, sbufferG);
	socket_send_packet(nodeSocket, bufferG, sbufferG);
	socket_close(nodeSocket);
	printf("Send OK\n"); // TODO handlear el error
	free(bufferG);
}
