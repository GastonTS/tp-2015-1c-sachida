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

	uint32_t sizeMap;
	memcpy(&sizeMap, buffer + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	sizeMap = ntohl(sizeMap);

	char* mapRutine = malloc(sizeof(char) * (sizeMap + 1));
	memcpy(mapRutine, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), sizeMap);
	mapRutine[sizeMap] = '\0';

	uint32_t sizeTmpName;
	memcpy(&sizeTmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeMap, sizeof(uint32_t));
	sizeTmpName = ntohl(sizeTmpName);

	char* tmpName = malloc(sizeof(char) * (sizeTmpName + 1));
	memcpy(tmpName, buffer + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeMap + sizeof(uint32_t), sizeTmpName);
	tmpName[sizeTmpName] = '\0';

	log_info(node_logger, "Job Requested MAP Rutine. numBlock %d. tmpName: %s", numBlock, tmpName);

	node_map(numBlock,mapRutine,tmpName);
	// OK

	// TODO
	bool ok = 1;
	socket_send_packet(socket, &ok, sizeof(ok));

	free(mapRutine);
	free(tmpName);
}

void connections_job_deserializeReduce(int socket, void *buffer) {
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

int node_map(uint16_t numBlock, char *mapRutine, char *tmpName){
	 //TODO aca habria que hacer un malloc de 20mb no?
	 char *blockData = node_getBlock(numBlock);
	 time_t tiempo = time(0);
	 struct tm *tlocal = localtime(&tiempo);
	 char date[13];
	 strftime(date,13,"%d%m%y%H%M%S",tlocal);
	 char path[28] = "/home/utnso/map";
	 memcpy(path+15, date ,13 );

	/*node_createExecutableFileFromString(path, mapRutine);
	popen_write(blockData,path);
	popen_read(path);*/

	//TODO usa sort para ordenar el temporal ya en el archivo definitivo  # cat map.py.result.tmp | sort > tmpNam
	free(blockData);
	/*ree(date);
	free(path);*/
	return 1;
 }

/*void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
 //el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
 El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino
 return 0;
 }*/

// popen() read
/*void popen_read(char *path) {
	char line[256];
	FILE *pipe = popen(path, "r");
	if (!pipe) {
		return; // TODO.
	}
	//fread(buffer, 1, 32, md5pipe);
	while (fgets(line, 255, pipe)) {
		//TODO aca tendria que escribirlo en el archivo temporal?
		printf("%s", line);
	}
	pclose(pipe);
}

// popen() write
void popen_write(char * blockData, char *path) {
	FILE *pipe = popen(path, "w");

	// fwrite() TODO, este es por bytes, creo que no va..
	fputs(blockData, pipe);
	pclose(pipe);
}

bool node_createExecutableFileFromString(char *pathToFile, char *str) {
	FILE *fp = fopen(pathToFile, "w");
	if (!fp) {
		return 0;
	}

	if (str) {
		fputs(str, fp);
	}

	int fd = fileno(fp);
	if (!fd) {
		fclose(fp);
		return 0;
	}

	struct stat st;
	if (fstat(fd, &st)) {
		fclose(fp);
		return 0;
	}

	// Sets exec mode.
	if (fchmod(fd, 0755)) {
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}*/
