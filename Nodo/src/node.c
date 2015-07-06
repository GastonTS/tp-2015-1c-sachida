#include "node.h"

#include "utils/socket.h"
#include "connections/connections_node.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

int node_initConfig(char* configFile);
void node_waitUntilExit();
bool node_init();
void node_free();

bool node_popen_write(char *command, char *data);
bool node_createExecutableFileFromString(char *pathToFile, char *str);

t_log *node_logger;
t_nodeCfg *node_config;

pthread_mutex_t *blocks_mutex = NULL;
void *binFileMap = NULL;

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
	log_info(node_logger, "Node Initialized successfully.");
	node_waitUntilExit();
	connections_shutdown();
	node_free();

	return EXIT_SUCCESS;
}

bool node_executeMapRutine(char *mapRutine, uint16_t numBlock, char *tmpFileName) {
	log_info(node_logger, "Executing MAP rutine on block number %d. Saving sorted to file in tmp dir as: %s", numBlock, tmpFileName);
	// First, get the block data..
	char *blockData = node_getBlock(numBlock);

	size_t commandSize;
	char *command;

	/************** WRITE ALL FILE PATHS. ******************/
	size_t pathToTmpFileSize = strlen(node_config->tmpDir) + 1 + strlen(tmpFileName) + 1;

	char pathToMapRutine[pathToTmpFileSize + 20];
	char pathToSTDERRFile[pathToTmpFileSize + 20];
	char pathToFinalSortedFile[pathToTmpFileSize];

	strcpy(pathToMapRutine, node_config->tmpDir);
	strcpy(pathToSTDERRFile, node_config->tmpDir);
	strcpy(pathToFinalSortedFile, node_config->tmpDir);

	strcat(pathToMapRutine, "/");
	strcat(pathToSTDERRFile, "/");
	strcat(pathToFinalSortedFile, "/");

	strcat(pathToMapRutine, tmpFileName);
	strcat(pathToSTDERRFile, tmpFileName);
	strcat(pathToFinalSortedFile, tmpFileName);

	strcat(pathToMapRutine, "_maprutine");
	strcat(pathToSTDERRFile, "_stderr");
	/************** WRITE ALL FILE PATHS. ******************/

	node_createExecutableFileFromString(pathToMapRutine, mapRutine);

	commandSize = strlen(pathToMapRutine) + 9 + strlen(pathToFinalSortedFile) + 3 + strlen(pathToSTDERRFile) + 1;
	command = malloc(commandSize);
	snprintf(command, commandSize, "%s | sort >%s 2>%s", pathToMapRutine, pathToFinalSortedFile, pathToSTDERRFile);

	bool result = node_popen_write(command, blockData);
	free(command);
	free(blockData);

	// TODO chech stderr ?

	return result;
}

bool node_executeReduceRutine(char *reduceRutine, char *tmpFilePathToReduce, char *finalFileName) {
	log_info(node_logger, "Executing REDUCE rutine to %s. Saving final result to file in tmp dir as: %s ", tmpFilePathToReduce, finalFileName);

	size_t commandSize;
	char *command;

	/************** WRITE ALL FILE PATHS. ******************/
	size_t pathToTmpFileSize = strlen(node_config->tmpDir) + 1 + strlen(finalFileName) + 1;

	char pathToReduceRutine[pathToTmpFileSize + 20];
	char pathToSTDERRFile[pathToTmpFileSize + 20];
	char pathToFinalFile[pathToTmpFileSize];

	strcpy(pathToReduceRutine, node_config->tmpDir);
	strcpy(pathToSTDERRFile, node_config->tmpDir);
	strcpy(pathToFinalFile, node_config->tmpDir);

	strcat(pathToReduceRutine, "/");
	strcat(pathToSTDERRFile, "/");
	strcat(pathToFinalFile, "/");

	strcat(pathToReduceRutine, finalFileName);
	strcat(pathToSTDERRFile, finalFileName);
	strcat(pathToFinalFile, finalFileName);

	strcat(pathToReduceRutine, "_reducerutine");
	strcat(pathToSTDERRFile, "_stderr");
	/************** WRITE ALL FILE PATHS. ******************/

	node_createExecutableFileFromString(pathToReduceRutine, reduceRutine);

	commandSize = 4 + strlen(node_config->tmpDir) + 1 + strlen(tmpFilePathToReduce) + 3 + strlen(pathToReduceRutine) + 2 + strlen(pathToFinalFile) + 3 + strlen(pathToSTDERRFile) + 1;
	command = malloc(commandSize);
	snprintf(command, commandSize, "cat %s/%s | %s >%s 2>%s", node_config->tmpDir, tmpFilePathToReduce, pathToReduceRutine, pathToFinalFile, pathToSTDERRFile);

	bool result = system(command) != -1;
	free(command);

	// TODO chech stderr ?

	return result;
}

char* node_getTmpFileContent(char *tmpFileName) {
	char pathToTmpFileName[strlen(node_config->tmpDir) + 1 + strlen(tmpFileName)];
	strcpy(pathToTmpFileName, node_config->tmpDir);
	strcat(pathToTmpFileName, "/");
	strcat(pathToTmpFileName, tmpFileName);

	int fd = open(pathToTmpFileName, O_RDONLY);
	if (fd == -1) {
		return NULL;
	}

	//Get the size of the file.
	struct stat stat;
	fstat(fd, &stat);

	char *fileMap = (char *) mmap(0, stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	char *fileStr = strdup(fileMap);

	munmap(fileMap, stat.st_size);
	close(fd);

	return fileStr;
}

// TODO no conviene hacer el map completo porque tira SEGAFULT,... nose que onda, hacer un mmap usando la parte del bloque y fue?
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

bool node_createExecutableFileFromString(char *pathToFile, char *str) {
	FILE *fp = fopen(pathToFile, "w");
	if (!fp) {
		return 0;
	}

	if (str) {
		fputs(str, fp);
	}

	int fd = fileno(fp);
	if (!fd) {
		fclose(fp);
		return 0;
	}

	struct stat st;
	if (fstat(fd, &st)) {
		fclose(fp);
		return 0;
	}

	// Sets exec mode.
	if (fchmod(fd, 0755)) {
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

bool node_popen_write(char *command, char *data) {
	FILE *pipe = popen(command, "w");
	if (!pipe) {
		free(command);
		return 0;
	}

	fputs(data, pipe);
	pclose(pipe);
	return 1;
}

bool node_init() {
	bool createFile = 0;
	bool binFileExists = access(node_config->binFilePath, F_OK) != -1;

	if (node_config->newNode || !binFileExists) {
		createFile = 1;
	}

	int fd;
	if (createFile) {
		fd = open(node_config->binFilePath, O_TRUNC | O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	} else {
		fd = open(node_config->binFilePath, O_RDWR);
	}

	if (fd == -1) {
		log_error(node_logger, "Error while trying to open the binFile.");
		return 0;
	}

	if (createFile) {
		if (ftruncate(fd, BLOCK_SIZE * node_config->blocksCount) == -1) {
			log_error(node_logger, "Error while trying to truncate the binFile.");
			return 0;
		}
	} else {
		// File exists, should check that size matches
		struct stat stat;
		fstat(fd, &stat);
		if (stat.st_size != BLOCK_SIZE * node_config->blocksCount) {
			log_error(node_logger, "You cannot change the size of the bin file if you are not a new node..");
			return 0;
		}
	}
	// ...

	// Map the file
	log_info(node_logger, "Mapping the binFile..");
	binFileMap = mmap(0, BLOCK_SIZE * node_config->blocksCount, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	close(fd);

	if (!binFileMap) {
		log_error(node_logger, "Error while trying to map the binFile.");
		return 0;
	}
	// ...

	// Create mutex for blocks
	blocks_mutex = malloc(sizeof(pthread_mutex_t) * node_config->blocksCount);
	int i;
	for (i = 0; i < node_config->blocksCount; i++) {
		if (pthread_mutex_init(&blocks_mutex[i], NULL) != 0) {
			log_error(node_logger, "Error while trying to create the block mutex.");
			return 0;
		}
	}
	// ...

	return 1;
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

void node_waitUntilExit() {
	pthread_mutex_t keepRunning;

	if (pthread_mutex_init(&keepRunning, NULL) != 0) {
		log_error(node_logger, "Error while trying to create new mutex: keepRunning");
		return;
	}

	void intHandler(int dummy) {
		pthread_mutex_unlock(&keepRunning);
	}

	signal(SIGINT, intHandler);
	pthread_mutex_lock(&keepRunning); // Locks it
	pthread_mutex_lock(&keepRunning); // Waits till it is unlocked.
	pthread_mutex_destroy(&keepRunning);
}

void node_free() {
	if (binFileMap) {
		munmap(binFileMap, BLOCK_SIZE * node_config->blocksCount);
	}

	if (node_config) {
		if (blocks_mutex) {
			int i;
			for (i = 0; i < node_config->blocksCount; i++) {
				pthread_mutex_destroy(&blocks_mutex[i]);
			}
			free(blocks_mutex);
		}

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
