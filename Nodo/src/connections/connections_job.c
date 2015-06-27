#include "connections_job.h"
#include "connections.h"

void* connections_job_listenActions(void *param);
void connections_job_deserializeMap(void *buffer);
void connections_job_deserializeReduce(void *buffer);

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
			connections_job_deserializeMap(buffer);
			break;
		case COMMAND_REDUCE:
			connections_job_deserializeReduce(buffer);
			break;
		default:
			log_error(node_logger, "JOB sent an invalid command %d", command);
			break;
		}
		free(buffer);
	}
}

void connections_job_deserializeMap(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint16_t sizeMap;
	memcpy(&sizeMap, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint16_t));
	sizeMap = ntohl(sizeMap);

	char* map = malloc(sizeof(char) * (sizeMap));
	memcpy(map, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), sizeMap);

	uint32_t sizeTmp;
	memcpy(&sizeTmp, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap, sizeof(uint16_t));
	sizeTmp = ntohl(sizeTmp);

	char* tmp = malloc(sizeof(char) * (sizeTmp));
	memcpy(tmp, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeMap + sizeof(uint16_t), sizeTmp);

	//node_setBlock(numBlock, blockData);
	//node_map();
	printf("llego al deseralize map\n");
	free(tmp);
	free(map);
}

void connections_job_deserializeReduce(void *buffer) {
	uint16_t numBlock;
	memcpy(&numBlock, buffer + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = ntohs(numBlock);

	uint16_t sizeReduce;
	memcpy(&sizeReduce, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint16_t));
	sizeReduce = ntohl(sizeReduce);

	char* reduce = malloc(sizeof(char) * (sizeReduce));
	memcpy(reduce, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint16_t), sizeReduce);
	//node_reduce();
	printf("llego al deseralize reduce\n");
	free(reduce);
}

/*void getFileContent(){
 Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
 getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
 por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
 es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.

 }*/

/*int nodeMap (rutinaMap, int nroBloque){
 El Nodo4 guarda el contenido de map.py en un archivo en el filesystem local. Le da permisos de ejecución.
 * El hilo mapper le solicita al Nodo4 que envie el contenido del Bloque6 por entrada estánda a map.py. El STDOUT lo almacena en un archivo temporal (ej: map.py.result.tmp)
 * Usa la tool sort para ordenar el archivo temporal del paso anterior ya en el archivo definitivo
 # cat map.py.result.tmp | sort > librazo12347.tmp
 * El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
 * los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper
 int lugarDeAlmacenamiento;
 return lugarDeAlmacenamiento;
 }*/

/*void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
 //el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
 El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino
 return 0;
 }*/
