#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "../test/PlanningTest.h"
#include "MaRTA.h"

typedef struct {
	int puerto_listen;
	char *ip_fs;
	int puerto_fs;
} t_configMaRTA;

t_configMaRTA *cfgMaRTA;
t_log *logger;
t_list *nodos;

int initConfig(char* archivoConfig);
void freeMaRTA();
void acceptJobs();

int main(int argc, char *argv[]) {

	logger = log_create("MaRTA.log", "MaRTA", 1, log_level_from_string("TRACE"));

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
	nodos = list_create();

	mapPlanningtest();

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

void acceptJobs() {

	int sockMaRTAfd, sockJobfd;
	struct sockaddr_in sockMaRTA;
	struct sockaddr_in sockJob;
	socklen_t sin_size;

	if ((sockMaRTAfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		log_error(logger, "Cant create listen socket");
		exit(-1);
	}

	sockMaRTA.sin_family = AF_INET;
	sockMaRTA.sin_port = htons(cfgMaRTA->puerto_listen);
	sockMaRTA.sin_addr.s_addr = INADDR_ANY;
	memset(&(sockMaRTA.sin_zero), 0, 8);

	if (bind(sockMaRTAfd, (struct sockaddr*) &sockMaRTA, sizeof(struct sockaddr)) == -1) {
		log_error(logger, "Cant brind listen socket");
		exit(-1);
	}

	if (listen(sockMaRTAfd, 5) == -1) {
		log_error(logger, "Cant listen");
		exit(-1);
	}

	while (1) {
		sin_size = sizeof(struct sockaddr_in);

		if ((sockJobfd = accept(sockMaRTAfd, (struct sockaddr *) &sockJob, &sin_size)) == -1) {
			log_error(logger, "Accept failed");
			exit(-1);

			//TODO: Crear nuevo hilo y funcion atenderJob()
		}
		log_info(logger, "Connected Job: %s", inet_ntoa(sockJob.sin_addr));
	}
}
