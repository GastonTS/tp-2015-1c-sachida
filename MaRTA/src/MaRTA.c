#include "MaRTA.h"
#include <commons/config.h>
#include "Connections/Connection.h"
#include "structs/node.h"

t_configMaRTA *cfgMaRTA;
t_log *logger;
t_list *nodes;
pthread_mutex_t McantJobs = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Mnodes = PTHREAD_MUTEX_INITIALIZER;
uint16_t cantJobs;

int initConfig(char* configFile);
void freeMaRTA();

int main(int argc, char *argv[]) {
	logger = log_create("MaRTA.log", "MaRTA", 1, log_level_from_string("TRACE"));
	cantJobs = 0;
	if (argc != 2) {
		log_error(logger, "Missing config file");
		freeMaRTA();
		return EXIT_FAILURE;
	}
	if (!initConfig(argv[1])) {
		log_error(logger, "Config failed");
		freeMaRTA();
		return EXIT_FAILURE;
	}

	nodes = list_create();
	signal(SIGINT, freeMaRTA);

	initConnection();

	list_destroy_and_destroy_elements(nodes, (void *) freeNode);
	freeMaRTA();
	return EXIT_SUCCESS;
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

	char* getConfigString(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(logger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(configFile);

	cfgMaRTA = malloc(sizeof(t_configMaRTA));

	log_info(logger, "Loading config...");

	cfgMaRTA->listenPort = getConfigInt("PUERTO_LISTEN");
	cfgMaRTA->fsIP = strdup(getConfigString("IP_FILE_SYSTEM"));
	cfgMaRTA->fsPort = getConfigInt("PUERTO_FILE_SYSTEM");

	if (!failure) {
		log_info(logger, "Port to listen: %d", cfgMaRTA->listenPort);
		log_info(logger, "FileSystem IP: %s", cfgMaRTA->fsIP);
		log_info(logger, "FileSystem Port: %d", cfgMaRTA->fsPort);
	}

	config_destroy(_config);
	return !failure;
}

void freeMaRTA() {
	if (cfgMaRTA->fsIP) {
		free(cfgMaRTA->fsIP);
	}
	free(cfgMaRTA);
	list_destroy_and_destroy_elements(nodes, (void *) freeNode);
	pthread_mutex_destroy(&McantJobs);
	pthread_mutex_destroy(&Mnodes);
	log_destroy(logger);
	exit(0);
}
