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
int fd;
int sock_job;
/* Estructuras */
int initConfig(char* configFile);

//void convertirListaArch(char* cadena,t_list* lista);


/* Nodo */
void desserializeMap(void* buffer);



int main(int argc, char *argv[]) {

	logger = log_create("Log.txt", "NODO", false, LOG_LEVEL_DEBUG);

		fd = socket_listen(30123);
		sock_job = socket_accept(fd);

		log_info(logger,"fd : %d", fd);
		log_info(logger,"conexion job: %d", sock_job);
		int hand = socket_handshake_to_client(sock_job, HANDSHAKE_NODO, HANDSHAKE_JOB);
		if (!hand) {
			printf("Error al conectar con Job \n");
			return EXIT_FAILURE;
		}

		void *buffer;
		size_t sbuffer = 0;
		int recibi;
		recibi = socket_recv_packet(sock_job, &buffer, &sbuffer);
		log_info(logger,"Recibi todo ok : %d\n",recibi);
		desserializeMap(buffer);
		/*
		recvOrder(fd);
		serializeMapResult(fd, true, 1);
		recvOrder(fd);
		char failedTemp[60];
		memset(failedTemp, '\0', sizeof(char) * 60);
		strcpy(failedTemp, "TemporalFallido");
		serializeReduceResult(fd, false, 0, failedTemp);
		recvOrder(fd);
		*/
		log_destroy(logger);
		return EXIT_SUCCESS;
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



//**********************************************************************************//
//									PAQUETES NODO 									//
//**********************************************************************************//

//**********************************Send Map Nodo************************************//

void desserializeMap(void* buffer){
	size_t sOrder = sizeof(char);
	size_t snumblock = sizeof(uint16_t);
	size_t sfilemap;
	size_t stempname;

	char* filemap;
	uint16_t numBlock;
	char* tempResultName;
	char order = '\0';

	log_info(logger,"Desserealizando");
	//order
	memcpy(&order,buffer,sOrder);
	log_info(logger,"order: %c",order);
	//numblock
	memcpy(&numBlock, buffer + sOrder, snumblock);
	numBlock = ntohs(numBlock);
	log_info(logger,"Block: %d",numBlock);
	//sizeArchivoMap
	memcpy(&sfilemap, buffer + sOrder + snumblock, sizeof(size_t));
	log_info(logger,"sfilemap: %d",sfilemap);
	//archivoMap
	filemap = malloc(sfilemap);
	memcpy(filemap, buffer + sOrder + snumblock + sizeof(size_t), sfilemap);
	log_info(logger,"Arch Map: %s",filemap);
	//sizeTempName
	memcpy(&stempname, buffer + sOrder + snumblock + sizeof(size_t) + sfilemap, sizeof(size_t));
	log_info(logger,"stemp: %d",stempname);
	//tempName
	tempResultName = malloc(stempname);
	memcpy(tempResultName, buffer + sOrder + snumblock + sizeof(size_t) + sfilemap + sizeof(size_t), stempname);
	log_info(logger,"Result: %s",tempResultName);

	//filemap = ntohs(filemap);
	//tempResultName = ntohs(tempResultName);



	//Test TODO: Adaptar a las estructuras de Job
	fflush(stdout);
	//free(filemap);
	//free(tempResultName);
	//End
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


