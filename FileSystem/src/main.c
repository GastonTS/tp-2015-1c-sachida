#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>

#include "mongo/mongo_file.h"
#include "mongo/mongo_dir.h"
#include "console/console.h"

void initConfig();
int getConfigInt(t_config* config, char *property);

int main(void) {
	initConfig();
	startConsole();
	return EXIT_SUCCESS;
}

void initConfig() {
	t_config* config;

	config = config_create("config.cfg");

	int minNodesCount = getConfigInt(config, "min-nodes-count");
	int port = getConfigInt(config, "port");

	printf("Config initialized: \n");
	printf("\t Minimum number of nodes to startup: %d \n", minNodesCount);
	printf("\t Port to listen: %d \n", port);

	config_destroy(config);
}

int getConfigInt(t_config* config, char *property) {
	if (config_has_property(config, property))
		return config_get_int_value(config, property);

	printf("Config not found for key %s \n", property);
	return NULL;
}
