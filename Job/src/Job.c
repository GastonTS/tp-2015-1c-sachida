/*
 * Job.c
 *
 *  Created on: 4/6/2015
 *      Author: utnso
 */

#include "structs/Job.h"
#include "utils/socket.h"

typedef struct {
	uint16_t PUERTO_MARTA;
	char* IP_MARTA;
	char* MAPPER;
	char* REDUCER;
	char* RESULTADO;
	char* LIST_ARCHIVOS;
	char*  COMBINER;
} t_configJob;

struct parms_threads{
	void *buffer;
	size_t tamanio;
};


t_configJob* cfgJob;
t_list* list_archivos;
t_list* list_mappers;
t_list* list_reducers;
pthread_t hilo_mapper;
pthread_t hilo_reduce;

int initConfig(char* configFile);
//void convertirListaArch(char* cadena,t_list* lista);
int conectarMarta();
void atenderMarta(int socketMarta);
void atenderMapper(void* parametros);
void confirmarMap();
void atenderReducer(void* parametros);
void confirmarReduce();
void freeJob();
void serializeConfigMaRTA(int fd, bool combiner, char* files);
void recvOrder(int fd);
void desserializeMapOrder(void *buffer);
void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer);
void desserializeReduceOrder(void *buffer, size_t sbuffer);

int main(int argc, char *argv[]) {

	logger = log_create("Log.txt", "JOB", false, LOG_LEVEL_DEBUG);

	list_mappers = list_create();
	list_reducers = list_create();
	list_archivos = list_create();

	if (argc != 2) {
		printf("ERROR -> La sintaxis es:  ./Job.c \"Ruta_archivo_config\" \n");
		return (-1);
	}

	if (!initConfig(argv[1])) {
		log_error(logger, "Config failed");
		freeJob();
		return EXIT_FAILURE;
	}

	sock_marta = conectarMarta();

	atenderMarta(sock_marta);

	freeJob();

	return 1;
}

void freeJob() {
	log_destroy(logger);
	free(cfgJob->IP_MARTA);
	free(cfgJob->MAPPER);
	free(cfgJob->REDUCER);
	free(cfgJob->RESULTADO);
	free(cfgJob->LIST_ARCHIVOS);
	free(cfgJob);
}

int initConfig(char* configFile) {

	t_config* _config;
	int failure = 0;

	int getConfigInt(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_int_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return -1;
	}

	char* getConfigString(char* property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(configFile);

	cfgJob = malloc(sizeof(t_configJob));
	cfgJob->PUERTO_MARTA = getConfigInt("PUERTO_MARTA");
	cfgJob->IP_MARTA = strdup(getConfigString("IP_MARTA"));
	cfgJob->MAPPER = strdup(getConfigString("MAPPER"));
	cfgJob->REDUCER = strdup(getConfigString("REDUCER"));
	cfgJob->RESULTADO = strdup(getConfigString("RESULTADO"));
	cfgJob->LIST_ARCHIVOS = strdup(getConfigString("LIST_ARCHIVOS"));
	cfgJob->COMBINER = strdup(getConfigString("COMBINER"));

	if (!failure) {
		log_info(logger, "PUERTO MARTA: %d", cfgJob->PUERTO_MARTA);
		log_info(logger, "IP MARTA: %s", cfgJob->IP_MARTA);
		log_info(logger, "MAPPER: %s", cfgJob->MAPPER);
		log_info(logger, "REDUCER: %s", cfgJob->REDUCER);
		log_info(logger, "RESULTADO: %s", cfgJob->RESULTADO);
		log_info(logger, "ARCHIVOS: %s", cfgJob->LIST_ARCHIVOS);
		log_info(logger, "COMBINER: %s", cfgJob->COMBINER);
	}

	config_destroy(_config);
	return !failure;
}

//**********************************************************************************//
//									CONEXIONES MaRTA											//
//**********************************************************************************//

int conectarMarta() {
	int handshake;

	if ((sock_marta = socket_connect(cfgJob->IP_MARTA, cfgJob->PUERTO_MARTA)) < 0) {
		log_error(logger, "Error al conectar con MaRTA %d", sock_marta);
		freeJob();
		return EXIT_FAILURE;
	}

	log_info(logger, "Coneccion con MaRTA: %d", sock_marta);

	handshake = socket_handshake_to_server(sock_marta, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	if (!handshake) {
		log_error(logger,"Error en handshake con MaRTA");
		freeJob();
		return EXIT_FAILURE;
	}

	return sock_marta;
}


void atenderMarta(int socketMarta) {

	/* Mando el Config a MaRTA */
	serializeConfigMaRTA(sock_marta, cfgJob->COMBINER, cfgJob->LIST_ARCHIVOS);

	while (1) {
		/*Recibo Orden de Map o Reduce */
		recvOrder(sock_marta);
	}
		/*
		memset(buffer, '\0', MAXDATASIZE);
		if ((numbytes = recv(socketMarta, buffer, SIZE_MSG, 0)) == -1) {
			printf("Error en el recv MapReduce de MaRTA \n");
			exit(-1);
		}
		memcpy(&mensaje, buffer, SIZE_MSG);

		if ((mensaje.id_proceso == MARTA) && (mensaje.tipo == MAP)) {
			//todo lo del mapper
			pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, NULL);
			//list_add(list_mappers,);

		} else if ((mensaje.id_proceso == MARTA) && (mensaje.tipo == REDUCE)) {
			//todo lo del reduce
			pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, NULL);
			//list_add(list_reducers,);

		} else {
			printf("Error en el recv MapReduce de MaRTA --> Me esta mandando otra cosa \n");
			free(buffer);
			close(socketMarta);
			exit(-1);
		}

	}*/

}

//**********************************************************************************//
//									PAQUETES MaRTA											//
//**********************************************************************************//

void serializeConfigMaRTA(int fd, bool combiner, char* stringFiles) {

	size_t filesLength = sizeof(stringFiles);
	size_t scombiner = sizeof(combiner);
	size_t sbuffer = scombiner + filesLength;
	void *buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	combiner = htons(combiner);
	memcpy(buffer, &combiner, scombiner);
	memcpy((buffer + scombiner), stringFiles, filesLength);

	socket_send_packet(fd, buffer, sbuffer);
	free(buffer);
}

void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	char order = '\0';
	size_t sOrder = sizeof(char);
	memcpy(&order, buffer, sOrder);

	/* Map */
	struct parms_threads parms_map;
	parms_map.buffer = (buffer + sOrder);
	parms_map.tamanio = sOrder;

	/* Reduce */
	struct parms_threads parms_reduce;
	parms_reduce.buffer = strdup(buffer + sOrder);
	parms_reduce.tamanio = sbuffer - sOrder;

	printf("\nRECVORDER: %c\n", order);
	if (order == 'm')
		pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, &parms_map);
	else if (order == 'r')
		pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, &parms_reduce);
		//desserializeReduceOrder(buffer + sOrder, sbuffer - sOrder);
	else if (order == 'd') {
		printf("\nDIE JOB\n");
		free(buffer);
		freeJob();
		return;
	}
}



void atenderMapper(void* parametros) {
	//struct parms_trheads* p = (struct parms_trheads*)parametros;
	//parms_threads p = (struct parms_threads*)parametros;
	struct parms_threads *p = (struct parms_threads *)parametros;

	log_info(logger, "Cree hilo mapper");

	/* Desserializo el mensaje de Mapper de MaRTA */
	desserializeMapOrder(p->buffer);
	//CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE MAP.PY

	/*
	int sockfd, numbytes;
	char* buffer;
	struct sockaddr_in nodo;
	int retorno = 0;


	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en conectar con NODO mapper\n");
		pthread_exit(&retorno);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Error en crear el socket del NODO mapper\n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	nodo.sin_family = AF_INET;
	//nodo.sin_port = htons(PUERTO_NODO);
	//nodo.sin_addr.s_addr = inet_addr(IP_NODO);
	memset(&(nodo.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr *) &nodo, sizeof(struct sockaddr)) == -1) {
		printf("Error en el connect con NODO mapper \n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = RUTINAMAP;
	mensaje.id_proceso = JOB;
	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sockfd, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send RUTINAMAP a NODO mapper \n");
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = BLOQUE;
	mensaje.id_proceso = JOB;
	//mensaje.datosNumericos = NUMERO_BLOQUE;
	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sockfd, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send BLOQUE a NODO mapper \n");
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	if ((numbytes = recv(sockfd, buffer, SIZE_MSG, 0)) == -1) {
		printf("Error en el recv confirmacion map de NODO \n");
		pthread_exit(&retorno);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje, buffer, SIZE_MSG);

	if ((mensaje.id_proceso == NODO) && (mensaje.tipo == MAPOK)) {
		log_info(logger, "Map terminado con exito ");
		confirmarMap();
	} else if ((mensaje.id_proceso == NODO) && (mensaje.tipo == MAPERROR)) {
		log_error(logger, "Map finalizado erroneamente");
		pthread_exit(&retorno);
	} else {
		printf("No recibi confirmacion del Nodo \n");
	}
	*/
}

void confirmarMap() {
	/*
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	int retorno = 0;

	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en confirmarMap a MaRTA\n");
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = MAPOK;
	mensaje.id_proceso = JOB;

	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sock_marta, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send CONFIRMARMAP a MaRTA \n");
		free(buffer);
		pthread_exit(&retorno);
	}

	free(buffer);
	pthread_exit(&retorno);
	*/
}

void atenderReducer(void* parametros) {
	log_info(logger, "Cree hilo reducer");

	//struct parms_trheads* p = (struct parms_trheads*)parametros;
	struct parms_threads *p = parametros;

	/* Desserializo el mensaje de Mapper de MaRTA */
	desserializeMapOrder(p->buffer);

	/*
	//CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE REDUCE.PY
	int sockfd, numbytes;
	char* buffer;
	int retorno = 0;
	struct sockaddr_in nodo;
	t_mensaje mensaje;

	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en conectar con NODO reduce \n");
		pthread_exit(&retorno);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("Error en crear el socket del NODO reduce\n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	nodo.sin_family = AF_INET;
	//nodo.sin_port = htons(PUERTO_NODO);
	//nodo.sin_addr.s_addr = inet_addr(IP_NODO);
	memset(&(nodo.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr *) &nodo, sizeof(struct sockaddr)) == -1) {
		printf("Error en el connect con NODO reduce \n");
		close(sockfd);
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = RUTINAREDUCE;
	mensaje.id_proceso = JOB;
	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sockfd, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send RUTINAREDUCE a NODO \n");
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	if ((numbytes = recv(sockfd, buffer, SIZE_MSG, 0)) == -1) {
		printf("Error en el recv confirmacion reduce de NODO \n");
		pthread_exit(&retorno);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje, buffer, SIZE_MSG);

	if ((mensaje.id_proceso == NODO) && (mensaje.tipo == REDUCEOK)) {
		log_info(logger, "Reduce terminado con exito ");
		confirmarReduce();
	} else if ((mensaje.id_proceso == NODO) && (mensaje.tipo == REDUCEERROR)) {
		log_error(logger, "Reduce finalizado erroneamente");
		pthread_exit(&retorno);
	} else {
		printf("No recibi confirmacion del Nodo \n");
	}
	*/
}

void confirmarReduce() {
	/*
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	int retorno = 0;

	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en confirmarReduce a MaRTA\n");
		pthread_exit(&retorno);
	}

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = REDUCEOK;
	mensaje.id_proceso = JOB;

	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sock_marta, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send CONFIRMARREDUCE a MaRTA \n");
		free(buffer);
		pthread_exit(&retorno);
	}

	free(buffer);
	pthread_exit(&retorno);
	*/
}

//***********************************MAP********************************************//
void desserializeMapOrder(void *buffer) {
	size_t sIdMap = sizeof(uint16_t);
	size_t snumblock = sIdMap;
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;
	uint16_t idMap;
	char* nodeIP;
	uint16_t nodePort;
	uint16_t numBlock;
	char tempResultName[60];

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, stempName);

	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	//Test TODO: Adaptar a las estructuras de Job
	printf("\n%d\n", idMap);
	printf("%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%d\n", numBlock);
	printf("%s\n", tempResultName);
	fflush(stdout);
	//End
}


//*********************************REDUCE*******************************************//
typedef struct {
	uint16_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char tempName[60];
} t_temp;

void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer) {
	t_temp *temporal = malloc(sizeof(t_temp));
	size_t snodeIP;

	memcpy(&temporal->originMap, buffer + *sbuffer, sizeof(uint16_t));
	temporal->originMap = ntohs(temporal->originMap);
	memcpy(&snodeIP, buffer + *sbuffer + sizeof(uint16_t), sizeof(snodeIP));
	temporal->nodeIP = malloc(snodeIP);
	memcpy(temporal->nodeIP, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP), snodeIP);
	memcpy(&temporal->nodePort, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP, sizeof(uint16_t));
	temporal->nodePort = ntohs(temporal->nodePort);
	memcpy(temporal->tempName, buffer + *sbuffer + sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t), sizeof(char) * 60);

	list_add(temporals, temporal);
	*sbuffer += sizeof(uint16_t) + sizeof(snodeIP) + snodeIP + sizeof(uint16_t) + sizeof(char) * 60;
}

void desserializeReduceOrder(void *buffer, size_t sbuffer) {
	size_t snodePort = sizeof(uint16_t);
	size_t stempName = sizeof(char) * 60;
	size_t snodeIP;

	char* nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
	uint16_t countTemps = 0;

	memcpy(&snodeIP, buffer, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sizeof(snodeIP), snodeIP);
	memcpy(&nodePort, buffer + sizeof(snodeIP) + snodeIP, snodePort);
	nodePort = ntohs(nodePort);
	memcpy(tempResultName, buffer + sizeof(snodeIP) + snodeIP + snodePort, stempName);
	memcpy(&countTemps, buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName, sizeof(uint16_t));
	countTemps = ntohs(countTemps);
	void *tempsBuffer = malloc(sbuffer - sizeof(snodeIP) - snodeIP - snodePort - stempName - sizeof(uint16_t));
	tempsBuffer = buffer + sizeof(snodeIP) + snodeIP + snodePort + stempName + sizeof(uint16_t);
	size_t stempsBuffer = 0;
	t_list *temps = list_create();
	for (; countTemps; countTemps--) {
		desserializeTempToList(temps, tempsBuffer, &stempsBuffer);
	}

//Test TODO: Adaptar a las estructuras de Job
	printf("\n%s\n", nodeIP);
	printf("%d\n", nodePort);
	printf("%s\n", tempResultName);
	printf("Count Temps:%d\n", list_size(temps));
	void showTemp(t_temp *temp) {
		printf("-Temp-\n");
		printf("\t%d\n", temp->originMap);
		printf("\t%s\n", temp->nodeIP);
		printf("\t%d\n", temp->nodePort);
		printf("\t%s\n", temp->tempName);
	}
	list_iterate(temps, (void *) showTemp);
//End
}

