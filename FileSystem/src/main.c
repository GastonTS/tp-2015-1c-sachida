#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>

#include "console/console.h"
#include "filesystem/filesystem.h"
#include "connections/connections.h"

bool initConfig(fs_connections_cfg_t *fsConfig);
void config_free(fs_connections_cfg_t *fsConfig);

int main(void) {
	fs_connections_cfg_t *fsConnectionsConfig = malloc(sizeof(fs_connections_cfg_t));

	if (!initConfig(fsConnectionsConfig)) {
		printf("Init failed\n");
		config_free(fsConnectionsConfig);
		return EXIT_FAILURE;
	}


	filesystem_initialize();
	connections_initialize(fsConnectionsConfig);
	console_start();
	connections_shutdown();
	filesystem_shutdown();

	config_free(fsConnectionsConfig);
	return EXIT_SUCCESS;
}

bool initConfig(fs_connections_cfg_t *fsConfig) {
	bool missing = 0;
	t_config* config;

	int getConfigInt(char *property) {
		if (config_has_property(config, property)) {
			return config_get_int_value(config, property);
		}

		missing = 1;
		printf("Config not found for key %s \n", property);
		return -1;
	}

	printf("Initializing MDFS. Getting config...\n");

	config = config_create("config.cfg");

	fsConfig->port = getConfigInt("PORT");
	fsConfig->minNodesCount = getConfigInt("MIN_NODES_COUNT");

	printf("\t Port to listen: %d \n", fsConfig->port);
	printf("\t Minimum number of nodes to startup: %d \n", fsConfig->minNodesCount);

	config_destroy(config);

	return !missing;
}

void config_free(fs_connections_cfg_t *fsConfig) {
	free(fsConfig);
}
