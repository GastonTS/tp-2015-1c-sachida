/*
 * Job.c
 *
 *  Created on: 4/6/2015
 *      Author: utnso
 */
/*

 LISTO 1--> Iniciar y extraer datos del archivo de configuracion
 LISTO 2--> Conectarse a MARTA
 Casi 3--> Mandarle a MARTA el/los archivo/s sobre el/los cual/es se desea trabajar.
 LISTO 4--> Quedar a la espera de ips y puertos de los nodos al cual me tengo que conectar para aplicar dichas rutinas
 NOTA:
 Cada rutina es un hilo nuevo, si es Map entonces es hilo mapper si es Reduce entonces hilo reduce
 Casi--> Conectarme al nodo X que me paso antes MARTA y mandarle la rutina que deseo correr "map.py" , el bloque
 y el archivo temporal donde debe ser almacenado
 Casi--> Esperar la finalizacion de la rutina y la confirmacion por parte del Nodo X
 Casi--> Notificar a MARTA de la ejecucion



 */

/* TODO
 VER COMO EXTRAER DEL ARCHIVO DE CONFIG LA LISTA DE ARCHIVOS
 */

#include "structs/Job.h"
#include "utils/socket.h"

typedef struct {
	uint16_t PUERTO_MARTA;
	char* IP_MARTA;
	char* MAPPER;
	char* REDUCER;
	char* RESULTADO;
	char* COMBINER;
} t_configJob;

t_configJob* cfgJob;
t_list* list_archivos;

int initConfig(char* configFile);
//void convertirListaArch(char* cadena,t_list* lista);
int conectarMarta();
void atenderMarta(int socketMarta);
void atenderMapper();
void confirmarMap();
void atenderReducer();
void confirmarReduce();
void freeJob();

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

	log_info(logger, "termine archivo");
	/*
	 int i;
	 char* cadena;
	 for (i=0;i< list_size(list_archivos);i++){
	 //cadena = malloc(strlen(list_get(list_archivos,i)));
	 //memset(cadena,'\0',strlen(list_get(list_archivos,i)));
	 //memcpy(list_get(list_archivos,i),cadena,strlen(list_get(list_archivos,i)));
	 strcpy(cadena,list_get(list_archivos,i));
	 log_info(logger,"Posic: %d String: %s",i,cadena);
	 }*/

	log_info(logger, "PUERTO MARTA: %d", cfgJob->PUERTO_MARTA);
	log_info(logger, "IP MARTA: %s", cfgJob->IP_MARTA);
	log_info(logger, "MAPPER: %s", cfgJob->MAPPER);
	log_info(logger, "REDUCER: %s", cfgJob->REDUCER);
	log_info(logger, "RESULTADO: %s", cfgJob->RESULTADO);
	log_info(logger, "COMBINER: %s", cfgJob->COMBINER);
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
	free(cfgJob->COMBINER);
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
	cfgJob->COMBINER = strdup(getConfigString("COMBINER"));

	if (!failure) {
		log_info(logger, "PUERTO MARTA: %d", cfgJob->PUERTO_MARTA);
		log_info(logger, "IP MARTA: %s", cfgJob->IP_MARTA);
		log_info(logger, "MAPPER: %s", cfgJob->MAPPER);
		log_info(logger, "REDUCER: %s", cfgJob->REDUCER);
		log_info(logger, "RESULTADO: %s", cfgJob->RESULTADO);
		log_info(logger, "COMBINER: %s", cfgJob->COMBINER);
	}

	config_destroy(_config);
	return !failure;
}

int conectarMarta() {
	int numbytes, handshake;
	char* buffer;
	t_mensaje mensaje;

	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en conectar con MaRTA \n");
		return (-1);
	}

	if ((sock_marta = socket_connect(cfgJob->IP_MARTA, cfgJob->PUERTO_MARTA)) < 0) {
		log_error(logger, "Error al conectar con MaRTA %d", sock_marta);
		freeJob();
	}

	log_info(logger, "Coneccion con MaRTA: %d", sock_marta);

	if ((handshake = socket_handshake_to_server(sock_marta, HANDSHAKE_MARTA, HANDSHAKE_JOB)) <= 0) {
		log_error(logger, "Error en el handshake con MaRTA: %d", handshake);
	}

	log_info(logger, "Handshake MaRTA: %d", handshake);

	memset(buffer, '\0', MAXDATASIZE);

	mensaje.tipo = ARCHIVOS;
	mensaje.id_proceso = JOB;
	//TODO CUANDO ARREGLE LO DE LA LISTA DE ARCHIVOS HAY QUE MANDARLA ACA
	//ADEMAS HAY QUE MANDARLE SI ES COMBINER O NO
	memcpy(buffer, &mensaje, SIZE_MSG);

	if ((numbytes = send(sock_marta, buffer, SIZE_MSG, 0)) <= 0) {
		printf("Error en el send archivos a MaRTA \n");
		free(buffer);
		return (-1);
	}

	free(buffer);
	return sock_marta;
}

void atenderMarta(int socketMarta) {
	int numbytes;
	char* buffer;
	t_mensaje mensaje;
	pthread_t hilo_mapper;
	pthread_t hilo_reduce;

	if ((buffer = (char*) malloc(sizeof(char) * MAXDATASIZE)) == NULL) {
		printf("Error al reservar memoria para el buffer en atender MaRTA \n");
		exit(-1);
	}

	while (1) {

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
	}

}

void atenderMapper() {

	log_info(logger, "Cree hilo mapper");

	//CONECTARME AL NODO Y MANDARLE EL BLOQUE Y LOS DATOS DE MAP.PY
	int sockfd, numbytes;
	char* buffer;
	struct sockaddr_in nodo;
	int retorno = 0;
	t_mensaje mensaje;

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

}

void confirmarMap() {
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
}

void atenderReducer() {
	log_info(logger, "Cree hilo reducer");

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
}

void confirmarReduce() {
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
}

