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

/*
typedef struct{
	uint16_t IdThread;
	pthread_t Thread;
} t_list_thread;
*/

typedef struct {
	uint16_t idJob;
	char *ip_nodo;
	uint16_t port_nodo;
	uint16_t numBlock;
	char *tempResultName;
} t_map;

//t_list_thread* hiloMap;
//t_list_thread* hiloRed;
t_configJob* cfgJob;
pthread_t hilo_mapper;
pthread_t hilo_reduce;
uint16_t contMap;
uint16_t contReduce;

/* Estructuras */
int initConfig(char* configFile);
void freeCfg();
void freeThreadMap(t_map* map);
//void convertirListaArch(char* cadena,t_list* lista);

/* MaRTA */
int conectarMarta();
void atenderMarta(int socketMarta);
void atenderMapper(void* parametros);
void confirmarMap();
void atenderReducer(void* parametros);
void confirmarReduce();

void serializeConfigMaRTA(int fd, bool combiner, char* files);
void recvOrder(int fd);
t_map* desserializeMapOrder(void *buffer);
void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer);
void desserializeReduceOrder(void *buffer, size_t sbuffer);

/* Nodo */
void serializeMap(int sock, t_map* map);
char* getMapRoutine(char* pathFile);


int main(int argc, char *argv[]) {

	logger = log_create("Log.txt", "JOB", false, LOG_LEVEL_DEBUG);

	if (argc != 2) {
		printf("ERROR -> La sintaxis es:  ./Job.c \"Ruta_archivo_config\" \n");
		return (-1);
	}

	if (!initConfig(argv[1])) {
		log_error(logger, "Config failed");
		freeCfg();
		return EXIT_FAILURE;
	}

	contMap = 0;
	contReduce = 0;

	if((sock_marta = conectarMarta()) >= 0){
		log_info(logger,"sock_marta: %d",sock_marta);
		atenderMarta(sock_marta);;
	}

	freeCfg();
	return 1;
}



//**********************************************************************************//
//									CONEXIONES MaRTA											//
//**********************************************************************************//

int conectarMarta() {
	int handshake;

	if ((sock_marta = socket_connect(cfgJob->IP_MARTA, cfgJob->PUERTO_MARTA)) < 0) {
		log_error(logger, "Error al conectar con MaRTA %d", sock_marta);
		freeCfg();
		return EXIT_FAILURE;
	}

	log_info(logger, "Coneccion con MaRTA: %d", sock_marta);

	handshake = socket_handshake_to_server(sock_marta, HANDSHAKE_MARTA, HANDSHAKE_JOB);
	if (!handshake) {
		log_error(logger,"Error en handshake con MaRTA");
		freeCfg();
		socket_close(sock_marta);
		return EXIT_FAILURE;
	}

	return sock_marta;
}


void atenderMarta(int socketMarta) {

	/* Mando el Config a MaRTA */
	serializeConfigMaRTA(sock_marta, cfgJob->COMBINER, cfgJob->LIST_ARCHIVOS);
	log_info(logger,"SerializeConfigMaRTA OK");
	while (1) {
		/*Recibo Orden de Map o Reduce */
		recvOrder(sock_marta);
	}
}

//**********************************************************************************//
//									PAQUETES MaRTA									//
//**********************************************************************************//

/* MANDO MIS CONFIGURACIONES INICIALES A MaRTA */
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

/* RECIBO ORDEN DE MAP O REDUCE DE MaRTA */
void recvOrder(int fd) {
	void *buffer;
	size_t sbuffer = 0;
	socket_recv_packet(fd, &buffer, &sbuffer);
	char order = '\0';
	size_t sOrder = sizeof(char);
	memcpy(&order, buffer, sOrder);
	log_info(logger,"recOrder");
	/* Map */
	struct parms_threads parms_map;
	parms_map.buffer = (buffer + sOrder);
	parms_map.tamanio = sOrder;

	/* Reduce */
	struct parms_threads parms_reduce;
	parms_reduce.buffer = (buffer + sOrder);
	parms_reduce.tamanio = sbuffer - sOrder;

	printf("\nRECVORDER: %c\n", order);
	if (order == 'm'){
		log_info(logger,"Map Recived");
		pthread_create(&hilo_mapper, NULL, (void*) atenderMapper, &parms_map);
		/*
		hiloMap = malloc(sizeof(t_list_thread));
		contMap++;
		hiloMap->IdThread = contMap;
		hiloMap->Thread = hilo_mapper;
		list_add(list_mappers,hiloMap);
		*/
	}
	else if (order == 'r'){
		log_info(logger,"Reduce Recived");
		pthread_create(&hilo_reduce, NULL, (void*) atenderReducer, &parms_reduce);
		/*
		hiloRed = malloc(sizeof(t_list_thread));
		contReduce++;
		hiloRed->IdThread = contReduce;
		hiloRed->Thread = hilo_reduce
		list_add(list_reducers,hiloRed);
		*/
	}
	else if (order == 'd') {
		printf("\nDIE JOB\n");
		free(buffer);
		freeCfg();
		exit(-1);
	}
}



void atenderMapper(void* parametros) {
	struct parms_threads *p = (struct parms_threads *)parametros;
	log_info(logger, "Thread map created");
	printf("Thread map created");

	t_map* map;

	/* Desserializo el mensaje de Mapper de MaRTA */

	map = desserializeMapOrder(p->buffer);


	/* Me conecto al Nodo */

	int hand_nodo;
	int sock_nodo;
	int ret_val = 0;

	if ((sock_nodo = socket_connect(map->ip_nodo, map->port_nodo)) < 0) {
		log_error(logger, "Error al conectar con Nodo %d", sock_nodo);
		freeThreadMap(map);
		pthread_exit(&ret_val);
	}

	log_info(logger, "Coneccion con Nodo: %d", sock_nodo);

	hand_nodo = socket_handshake_to_server(sock_nodo, HANDSHAKE_NODO, HANDSHAKE_JOB);
	if (!hand_nodo) {
		log_error(logger,"Error en hand_nodo con Nodo %d", hand_nodo);
		freeThreadMap(map);
		pthread_exit(&ret_val);
	}

	/* Serializo y mando datos */
	log_info(logger,"Handshake Nodo: %d",hand_nodo);
	serializeMap(sock_nodo,map);

}

void confirmarMap() {

}

void atenderReducer(void* parametros) {
	log_info(logger, "Cree hilo reducer");

	//struct parms_trheads* p = (struct parms_trheads*)parametros;
	struct parms_threads *p = parametros;

	/* Desserializo el mensaje de Mapper de MaRTA */
	desserializeMapOrder(p->buffer);

	//TODO CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE REDUCE.PY

}

void confirmarReduce() {

}

//**********************************************************************************//
//									PAQUETES MaRTA 									//
//**********************************************************************************//

//***********************************MAP********************************************//
t_map* desserializeMapOrder(void *buffer) {
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
	t_map* map;
	map = malloc(sizeof(t_map));

	memcpy(&idMap, buffer, sIdMap);
	memcpy(&snodeIP, buffer + sIdMap, sizeof(size_t));
	nodeIP = malloc(snodeIP);
	memcpy(nodeIP, buffer + sIdMap + sizeof(size_t), snodeIP);
	memcpy(&nodePort, buffer + sIdMap + sizeof(size_t) + snodeIP, snodePort);
	memcpy(&numBlock, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort, snumblock);
	memcpy(tempResultName, buffer + sIdMap + sizeof(size_t) + snodeIP + snodePort + snumblock, stempName);

	//TODO GUARDAR EN ESTRUCTURAS
	idMap = ntohs(idMap);
	nodePort = ntohs(nodePort);
	numBlock = ntohs(numBlock);

	map = malloc(sizeof(t_map));
	map->idJob = idMap;
	map->ip_nodo = strdup(nodeIP);
	map->port_nodo = nodePort;
	map->numBlock = numBlock;
	map->tempResultName = strdup(tempResultName);


	printf("\nID: %d\n", map->idJob);
	printf("IP:  %s  \n", map->ip_nodo);
	printf("PUERTO: %d\n", map->port_nodo);
	printf("NUMBLOCK: %d\n", map->numBlock);
	printf("%s\n", map->tempResultName);
	fflush(stdout);
	free(nodeIP);
	return(map);

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

//**********************************************************************************//
//									PAQUETES NODO 									//
//**********************************************************************************//

//**********************************Send Map Nodo************************************//

void serializeMap(int sock_nodo, t_map* map){
	char order = 'm';
	size_t sOrder = sizeof(char);
	size_t sBlock = sizeof(uint16_t);
	char* fileMap;
	size_t sTempName = strlen(map->tempResultName);

	/* Obtenemos binario de File Map */
	fileMap = getMapRoutine(cfgJob->MAPPER);
	size_t sfileMap = strlen(fileMap);

	uint16_t numBlock = htons(map->numBlock);

	/* Armo el paquete y lo mando */

	log_info(logger,"Block: %d",numBlock);
	log_info(logger,"fileMap: %s", cfgJob->MAPPER);
	log_info(logger,"tempResultName: %s", map->tempResultName);
	size_t sbuffer = sOrder + sBlock + sfileMap + sTempName;
	void* buffer = malloc(sbuffer);
	buffer = memset(buffer, '\0', sbuffer);
	memcpy(buffer, &order, sOrder);
	memcpy(buffer + sOrder, &numBlock, sBlock);
	memcpy(buffer + sOrder + sBlock, &sfileMap, sizeof(uint16_t));
	memcpy(buffer + sOrder + sBlock + sizeof(uint16_t), fileMap, sfileMap);
	memcpy(buffer + sOrder + sBlock + sizeof(uint16_t), &sTempName, sizeof(uint16_t));
	memcpy(buffer + sOrder + sBlock + sizeof(uint16_t) + sfileMap + sizeof(uint16_t), map->tempResultName, sTempName);

	socket_send_packet(sock_nodo,buffer,sbuffer);
	log_info(logger,"Enviado");
	free(fileMap);
	free(buffer);

}


//**********************************************************************************//
//									ESTRUCTURAS  									//
//**********************************************************************************//

//**********************************Free Job****************************************//

void freeCfg() {
	log_destroy(logger);
	free(cfgJob->IP_MARTA);
	free(cfgJob->MAPPER);
	free(cfgJob->REDUCER);
	free(cfgJob->RESULTADO);
	free(cfgJob->LIST_ARCHIVOS);
	free(cfgJob);
}

//**********************************Free Thread Map****************************************//
void freeThreadMap(t_map* map){
	free(map->ip_nodo);
	free(map->tempResultName);
	free(map);
}

//**********************************Init Config****************************************//
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


/*******************************Get Map Routine*************************************/
char* getMapRoutine(char* pathFile){
	int mapper;
	char* mapeo;
	int size;

		/* Get size of File */
		int size_of(int fd){
			struct stat buf;
			fstat(fd, &buf);
			return buf.st_size;
		}

	mapper = open (pathFile, O_RDONLY);
	size = size_of(mapper);
	if( (mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper,0)) == MAP_FAILED){
		//Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
		fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", pathFile, size, strerror(errno));
		abort();
	}
	//Se unmapea , y se cierrra el archivo
	//munmap( mapeo, size );
	close(mapper);
	return mapeo;
}
