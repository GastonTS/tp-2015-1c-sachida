#include <stdio.h>
#include <stdlib.h>
#include <commons/config.h>

#include "mongo/mongo_file.h"
#include "mongo/mongo_dir.h"
#include "console/console.h"
#include "filesystem/filesystem.h"

typedef struct {
	int *port;
	int *minNodesCount;
} fscfg_t;

bool initConfig(fscfg_t *fsConfig);
void config_free(fscfg_t *fsConfig);

void testNode();

int main(void) {
	fscfg_t *fsConfig = malloc(sizeof(fscfg_t));

	if (!initConfig(fsConfig)) {
		printf("Init failed\n");
		config_free(fsConfig);
		return EXIT_FAILURE;
	}

	testNode();

	filesystem_initialize();
	console_start();
	filesystem_shutdown();

	config_free(fsConfig);
	return EXIT_SUCCESS;
}

void testNode() {
	if (1) {
		node_t* node = node_create(50);

		node->name = strdup("Node3");
		if (0) {
			node_setBlockUsed(node, 1);
			node_setBlockUsed(node, 2);
			node_setBlockUsed(node, 23);
			node_setBlockUsed(node, 10);
			node_setBlockUsed(node, 15);
			node_setBlockUsed(node, 18);
			node_setBlockUsed(node, 6);
			node_setBlockUsed(node, 7);
		}
		mongo_node_save(node);

		if (0) {
			node_t* node2 = mongo_node_getById("55773bb9508b9631cc542ab1");

			node_printBlocksStatus(node2);
			node_setBlockUsed(node2, 3);
			mongo_node_updateBlocks(node2);
		}
	}
}

bool initConfig(fscfg_t *fsConfig) {
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

	fsConfig->port = malloc(sizeof(int));
	fsConfig->minNodesCount = malloc(sizeof(int));

	*fsConfig->port = getConfigInt("PORT");
	*fsConfig->minNodesCount = getConfigInt("MIN_NODES_COUNT");

	printf("\t Port to listen: %d \n", *fsConfig->port);
	printf("\t Minimum number of nodes to startup: %d \n", *fsConfig->minNodesCount);

	config_destroy(config);

	return !missing;
}

void config_free(fscfg_t *fsConfig) {
	free(fsConfig->port);
	free(fsConfig->minNodesCount);
	free(fsConfig);
}
