#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>
#include <commons/log.h>
#include <commons/bitarray.h>
#include "structs/job.h"
#include "structs/nodo.h"

typedef struct {
	int puerto_listen;
	char *ip_fs;
	int puerto_fs;
} t_configMaRTA;

t_configMaRTA* cfgMaRTA;
t_log* logger;

int initConfig(char* archivoConfig);
void freeMaRTA();

int main(int argc, char *argv[]) {

	logger = log_create("MaRTA.log", "MaRTA", 1,
			log_level_from_string("TRACE"));

	if (argc != 2) {
		log_error(logger, "Falta archivo de configuracion en la invocacion");
		freeMaRTA();
		return EXIT_FAILURE;
	}

	if (!initConfig(argv[1])) {
		log_error(logger, "Fallo configuracion");
		freeMaRTA();
		return EXIT_FAILURE;
	}

	freeMaRTA();
	return EXIT_SUCCESS;
}

int initConfig(char* archivoConfig) {

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

	char* getCongifString(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(archivoConfig);

	cfgMaRTA = malloc(sizeof(t_configMaRTA));

	log_info(logger, "Iniciando configuracion...");

	cfgMaRTA->puerto_listen = getConfigInt("PUERTO_LISTEN");
	cfgMaRTA->ip_fs = getCongifString("IP_FILE_SYSTEM");
	cfgMaRTA->puerto_fs = getConfigInt("PUERTO_FILE_SYSTEM");

	if (!failure) {
		log_info(logger, "Port to listen: %d", cfgMaRTA->puerto_listen);
		log_info(logger, "IP FileSystem: %s", cfgMaRTA->ip_fs);
		log_info(logger, "FileSystem Port: %d", cfgMaRTA->puerto_fs);
	}

	config_destroy(_config);

	return !failure;
}

void freeMaRTA() {
	free(cfgMaRTA);
	log_destroy(logger);
}

