#include "node.h"

#include "utils/socket.h"
#include "connections/connections.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

int node_initConfig(char* configFile);
void node_readCommand();
bool node_init();
void node_free();

t_log *node_logger;
t_nodeCfg *node_config;

pthread_mutex_t *blocks_mutex;
void *binFileMap;

int main(int argc, char *argv[]) {
	node_logger = log_create("node.log", "Node", 1, log_level_from_string("TRACE"));

	if (argc != 2) {
		log_error(node_logger, "Missing config file");
		node_free();
		return EXIT_FAILURE;
	}

	node_config = malloc(sizeof(t_nodeCfg));

	if (!node_initConfig(argv[1])) {
		log_error(node_logger, "Config failed");
		node_free();
		return EXIT_FAILURE;
	}

	if (!node_init()) {
		log_error(node_logger, "Failed to initialize node.");
		node_free();
		return EXIT_FAILURE;
	}

	connections_initialize();
	log_info(node_logger, "Node Initialized, press 'd' and enter to exit.");
	node_readCommand(); // Waits till 'd' is pressed
	connections_shutdown();
	node_free();

	return EXIT_SUCCESS;
}

void node_readCommand() {
	int c;
	//system("/bin/stty raw");
	while ((c = getchar()) != 'd') {
	}
	//system("/bin/stty cooked");
}

bool node_init() {
	bool createFile = 0;
	bool binFileExists = access(node_config->binFilePath, F_OK) != -1;

	if (node_config->newNode) {
		createFile = 1;
	} else {
		if (!binFileExists) {
			// If it is not a new node but the file does not exist, then I create it. Otherwise it will be kept intact.
			createFile = 1;
		}
	}

	if (createFile) {
		int fd = open(node_config->binFilePath, O_TRUNC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			return 0;
		}

		if (ftruncate(fd, BLOCK_SIZE * node_config->blocksCount) == -1) {
			return 0;
		}

		close(fd);
	} else {
		//fd = open(node_config->binFilePath, O_TRUNC); // Truncate just in case the blokcsCount changed..
	}
	// ...

	// Map the file
	log_info(node_logger, "Mapping the binFile..");

	int fd = open(node_config->binFilePath, O_RDWR);
	if (fd == -1) {
		log_error(node_logger, "An error occurred while trying to open the bin file.");
		return 0;
	}
	binFileMap = mmap(0, BLOCK_SIZE * node_config->blocksCount, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	close(fd);
	// ...

	// Create mutex for blocks
	blocks_mutex = malloc(sizeof(pthread_mutex_t) * node_config->blocksCount);
	int i;
	for (i = 0; i < node_config->blocksCount; i++) {
		if (pthread_mutex_init(&blocks_mutex[i], NULL) != 0) {
			return 0;
		}
	}
	// ...

	return 1;
}

char* node_getBlock(uint16_t numBlock) {
	log_info(node_logger, "Getting block number %d", numBlock);

	char *blockStr = malloc(sizeof(char) * BLOCK_SIZE);

	pthread_mutex_lock(&blocks_mutex[numBlock]);
	memcpy(blockStr, binFileMap + (numBlock * BLOCK_SIZE), BLOCK_SIZE);
	pthread_mutex_unlock(&blocks_mutex[numBlock]);

	return blockStr;
}

void node_setBlock(uint16_t numBlock, char *blockStr) {
	log_info(node_logger, "Setting block number %d", numBlock);

	pthread_mutex_lock(&blocks_mutex[numBlock]);
	memcpy(binFileMap + (numBlock * BLOCK_SIZE), blockStr, strlen(blockStr) + 1);
	pthread_mutex_unlock(&blocks_mutex[numBlock]);
}

int node_initConfig(char* configFile) {
	t_config* config;
	int failure = 0;

	int getConfigInt(char *property) {
		if (config_has_property(config, property)) {
			return config_get_int_value(config, property);
		}

		failure = 1;
		log_error(node_logger, "Config not found for key %s", property);
		return -1;
	}

	char* getConfigString(char *property) {
		if (config_has_property(config, property)) {
			return config_get_string_value(config, property);
		}

		failure = 1;
		log_error(node_logger, "Config not found for key %s", property);
		return "";
	}

	config = config_create(configFile);

	log_info(node_logger, "Loading config...");

	node_config->fsIp = strdup(getConfigString("FS_IP"));
	node_config->fsPort = getConfigInt("FS_PORT");
	node_config->listenPort = getConfigInt("LISTEN_PORT");
	node_config->binFilePath = strdup(getConfigString("BIN_FILE_PATH"));
	node_config->newNode = (bool) getConfigInt("NEW_NODE");
	node_config->blocksCount = getConfigInt("BLOCKS_COUNT");
	node_config->name = strdup(getConfigString("NAME"));
	node_config->tmpDir = strdup(getConfigString("TMP_DIR"));

	if (!failure) {
		log_info(node_logger, "My name is: %s", node_config->name);
		log_info(node_logger, "am I new?: %s", node_config->newNode ? "yes" : "no");
		log_info(node_logger, "Blocks count: %d", node_config->blocksCount);
		log_info(node_logger, "Bin file path: %s", node_config->binFilePath);
		log_info(node_logger, "Temp dir: %s", node_config->tmpDir);
		log_info(node_logger, "FileSystem IP: %s", node_config->fsIp);
		log_info(node_logger, "FileSystem Port: %d", node_config->fsPort);
		log_info(node_logger, "Listening port: %d", node_config->listenPort);
	}

	config_destroy(config);
	return !failure;
}

void node_free() {
	munmap(binFileMap, BLOCK_SIZE * node_config->blocksCount);

	int i;
	for (i = 0; i < node_config->blocksCount; i++) {
		pthread_mutex_destroy(&blocks_mutex[i]);
	}

	if (node_config) {
		if (node_config->fsIp) {
			free(node_config->fsIp);
		}
		if (node_config->binFilePath) {
			free(node_config->binFilePath);
		}
		if (node_config->tmpDir) {
			free(node_config->tmpDir);
		}
		if (node_config->name) {
			free(node_config->name);
		}
		free(node_config);
	}
	log_destroy(node_logger);
}
