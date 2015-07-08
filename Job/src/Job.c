/*
 * Job.c
 *
 *  Created on: 4/6/2015
 *      Author: utnso
 */

#include "structs/Job.h"
#include "utils/socket.h"
#include "structs/node.h"
#include "marta/marta.h"

int main(int argc, char *argv[]) {

	logger = log_create("Job.log", "JOB", 1, LOG_LEVEL_DEBUG);

	if (argc != 2) {
		printf("ERROR -> La sintaxis es:  ./Job.c \"Ruta_archivo_config\" \n");
		return EXIT_FAILURE;
	}

	if (!initConfig(argv[1])) {
		log_error(logger, "Config failed");
		freeCfg();
		return EXIT_FAILURE;
	}

	if (pthread_mutex_init(&Msockmarta, NULL) != 0) {
		log_error(logger, "ERROR - No se pudo inicializar el mutex Msockmarta");
		return 1;
	}

	if ((sock_marta = conectarMarta()) >= 0) {
		log_info(logger, "sock_marta: %d", sock_marta);
		atenderMarta(sock_marta);
		;
	}

	return EXIT_SUCCESS;
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
void freeThreadMap(t_map* map) {
	free(map->ip_nodo);
	free(map->tempResultName);
	free(map);
}

//**********************************Free Thread Reduce**************************************//
void freeThreadReduce(t_reduce* reduce) {
	free(reduce->ip_nodo);
	free(reduce->tempResultName);
	free(reduce->buffer_tmps);
	free(reduce);
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
	cfgJob->COMBINER = getConfigInt("COMBINER");

	if (!failure) {
		log_info(logger, "PUERTO MARTA: %d", cfgJob->PUERTO_MARTA);
		log_info(logger, "IP MARTA: %s", cfgJob->IP_MARTA);
		log_info(logger, "MAPPER: %s", cfgJob->MAPPER);
		log_info(logger, "REDUCER: %s", cfgJob->REDUCER);
		log_info(logger, "RESULTADO: %s", cfgJob->RESULTADO);
		log_info(logger, "ARCHIVOS: %s", cfgJob->LIST_ARCHIVOS);
		log_info(logger, "COMBINER: %d", cfgJob->COMBINER);
	}

	config_destroy(_config);
	return !failure;
}

/*******************************Get Map Routine*************************************/
char* getMapReduceRoutine(char* pathFile) {
	int mapper;
	char* mapeo;
	int size;

	/* Get size of File */
	int size_of(int fd) {
		struct stat buf;
		fstat(fd, &buf);
		return buf.st_size;
	}

	mapper = open(pathFile, O_RDONLY);
	size = size_of(mapper);
	if ((mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper, 0))
			== MAP_FAILED) {
		//Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
		fprintf(stderr,
				"Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size",
				pathFile, size, strerror(errno));
		abort();
	}
	//Se unmapea , y se cierrra el archivo
	//munmap( mapeo, size );
	close(mapper);
	return mapeo;
}
