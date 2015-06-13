#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "structs/node.h"
#include "structs/job.h"
#include "Serialize/serialize.h"
#include "../test/PlanningTest.h"
#include "MaRTA.h"
#include "../../utils/socket.h"

typedef struct {
	uint16_t listenPort;
	char *fsIP;
	uint16_t fsPort;
} t_configMaRTA;

t_configMaRTA *cfgMaRTA;
t_log *logger;
t_list *nodes;

int fdListener;
int fdAccepted;
bool exitMaRTA;

int initConfig(char* configFile);
void freeMaRTA();
uint32_t cantJobs;

int main(int argc, char *argv[]) {

	logger = log_create("MaRTA.log", "MaRTA", 1, log_level_from_string("TRACE"));
	exitMaRTA = false;
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

	fdListener = socket_listen(cfgMaRTA->listenPort);
	while (!exitMaRTA) {
		printf("Waiting conecttion...\n");
		fflush(stdout);
		fdAccepted = socket_accept(fdListener);
		switch (socket_handshake_to_client(fdAccepted, HANDSHAKE_MARTA, HANDSHAKE_FILESYSTEM | HANDSHAKE_JOB)) {
		case HANDSHAKE_FILESYSTEM:
			printf("\nEl FileSystem!\n");
			break;
		case HANDSHAKE_JOB:
			cantJobs++;
			//TODO levantar un hilo
			t_job *job = desserealizeJob(fdAccepted, cantJobs);

			if (job->combiner)
				log_info(logger, "Iniciando Job: %d (Combiner)", job->id);
			else
				log_info(logger, "Iniciando Job: %d (No combiner)", job->id);

			t_map *map=malloc(sizeof(t_map));
			map->id = 42;
			map->numBlock = 13;
			map->nodePort = 30123;
			map->nodeIP = "x.y.z.w";
			strcpy(map->tempResultName, "sarasaaaaaaaaaaaa.txt");
			serialiceMapToOrder(fdAccepted, map);
			freeJob(job);

			if (cantJobs == 3) //XXX
				exitMaRTA = 1;
			break;
		}
	}
	fflush(stdout);

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

	char* getCongifString(char *property) {
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
	cfgMaRTA->fsIP = getCongifString("IP_FILE_SYSTEM");
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
	free(cfgMaRTA);
	log_destroy(logger);
}
