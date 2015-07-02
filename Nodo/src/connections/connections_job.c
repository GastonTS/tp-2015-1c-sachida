#include "connections_job.h"
#include "connections.h"
#include "../node.h"

void* connections_job_listenActions(void *param);
void connections_job_deserializeMap(int socket, void *buffer);
void connections_job_deserializeReduce(int socket, void *buffer);
void popen_read(char *path);
void popen_write(char *blockData, char *path);
int node_map(uint16_t numBlock, char *mapRutine, char *tmpName);

void connections_job_initialize() {

}

void connections_job_shutdown() {

}

void* connections_job_accept(void *param) {
	int *socketAcceptedPtr = (int *) param;

	log_info(node_logger, "New job connected.");

	pthread_t listenActionsTh;
	if (pthread_create(&listenActionsTh, NULL, (void *) connections_job_listenActions, (void *) socketAcceptedPtr)) {
		free(socketAcceptedPtr);
		log_error(node_logger, "Error while trying to create new thread: connections_job_listenActions");
	}
	pthread_detach(listenActionsTh);

	return NULL;
}

void* connections_job_listenActions(void *param) {
	int *socketAcceptedPtr = (int *) param;
	int socket = *socketAcceptedPtr;
	free(socketAcceptedPtr);

	while (1) {
		size_t sBuffer;
		void *buffer = NULL;

		e_socket_status status = socket_recv_packet(socket, &buffer, &sBuffer);
		if (0 > status) {
			socket_close(socket);
			return NULL;
		}

		uint8_t command;
		memcpy(&command, buffer, sizeof(uint8_t));

		switch (command) {
		case COMMAND_MAP:
			connections_job_deserializeMap(socket, buffer);
			break;
		case COMMAND_REDUCE:
			connections_job_deserializeReduce(socket, buffer);
			break;
		default:
			log_error(node_logger, "JOB sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_job_deserializeMap(int socket, void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sMapRutine;
	memcpy(&sMapRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sMapRutine = ntohl(sMapRutine);

	char* mapRutine = malloc(sizeof(char) * (sMapRutine + 1));
	memcpy(mapRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sMapRutine);
	mapRutine[sMapRutine] = '\0';

	uint32_t sTmpName;
	memcpy(&sTmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sMapRutine, sizeof(uint32_t));
	sTmpName = ntohl(sTmpName);

	char* tmpName = malloc(sizeof(char) * (sTmpName + 1));
	memcpy(tmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sMapRutine + sizeof(uint32_t), sTmpName);
	tmpName[sTmpName] = '\0';

	log_info(node_logger, "Job Requested MAP Rutine. numBlock %d. tmpName: %s", numBlock, tmpName);

	bool ok = node_executeMapRutine(mapRutine, numBlock, tmpName);

	socket_send_packet(socket, &ok, sizeof(ok));

	free(mapRutine);
	free(tmpName);
}

void connections_job_deserializeReduce(int socket, void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint32_t sReduceRutine; // TODO , avisar a JOB que esto va a ser de 32 bits
	memcpy(&sReduceRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sReduceRutine = ntohl(sReduceRutine);

	char* reduceRutine = malloc(sizeof(char) * (sReduceRutine + 1));
	memcpy(reduceRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sReduceRutine);
	reduceRutine[sReduceRutine] = '\0';

	node_executeReduceRutine(reduceRutine, numBlock);

	free(reduceRutine);
}

/*void getFileContent(){
 Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
 getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
 por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
 es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.

 }*/

/*void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
 //el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
 El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino
 return 0;
 }*/

