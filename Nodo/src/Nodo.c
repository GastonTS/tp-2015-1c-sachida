#include "Nodo.h"
#include "utils/socket.h"
#include "connections/connections.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

int node_initConfig(char* configFile);
void node_free();

t_log *node_logger;
t_nodeCfg *node_config;

//Le agregue los argumentos para que se pueda pasar el archivo de conf como parametro del main
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

	// TODO move.
	if (node_config->newNode) {
		int fd = open("/home/utnso/block", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			return -1; // TODO handlear.
		}

		// TODO truncate -s 1G miarchivo.bin*/
		lseek(fd, BLOCK_SIZE * node_config->blocksCount, SEEK_SET);
		write(fd, "", 1);
		lseek(fd, 0, SEEK_SET);

		close(fd);
	}

	connections_initialize();
	while (1) {
		// TODO DELETE
	}
	node_free();
	return EXIT_SUCCESS;
}

char* node_getBlock(uint16_t numBlock) {
	log_info(node_logger, "Getting block number %d", numBlock);

	int fd = open(node_config->binFilePath, O_RDONLY);
	if (fd == -1) {
		log_error(node_logger, "An error occurred while trying to open the bin file.");
		return NULL;
	}

	char *blockStr = (char *) mmap(0, BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, numBlock * BLOCK_SIZE);

	close(fd);

	return blockStr;
}

void node_freeBlock(char *blockStr) {
	munmap(blockStr, BLOCK_SIZE);
}

void node_setBlock(uint16_t numBlock, char *blockData) {
	log_info(node_logger, "Setting block number %d", numBlock);

	int fd = open(node_config->binFilePath, O_RDWR);
	if (fd == -1) {
		log_error(node_logger, "An error occurred while trying to open the bin file.");
		return;
	}

	char *fileBlockStr = (char *) mmap(0, BLOCK_SIZE, PROT_WRITE, MAP_SHARED, fd, numBlock * BLOCK_SIZE);

	memcpy(fileBlockStr, blockData, strlen(blockData) + 1);

	munmap(fileBlockStr, BLOCK_SIZE);
	close(fd);
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
