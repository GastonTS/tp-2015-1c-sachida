#include "node.h"

#include "utils/socket.h"
#include "connections/connections_node.h"

#define _FILE_OFFSET_BITS  64

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/errno.h>

int node_initConfig(char* configFile);
void node_waitUntilExit();
bool node_init();
void node_free();

bool node_popen_write(char *command, char *data);
bool node_createExecutableFileFromString(char *pathToFile, char *str);

t_log *node_logger;
t_nodeCfg *node_config;

pthread_rwlock_t *blocks_mutex = NULL;
void *binFileMap = NULL;

int main(int argc, char *argv[]) {
	node_logger = log_create("node.log", "Node", 1, log_level_from_string("TRACE"));

	if (argc != 2) {
		log_error(node_logger, "Missing config file");
		node_free();
		return EXIT_FAILURE;
	}

	node_config = malloc(sizeof(t_nodeCfg));
	node_config->binFilePath = NULL;
	node_config->fsIp = NULL;
	node_config->name = NULL;
	node_config->tmpDir = NULL;

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

	bool fileCreated = node_createExecutableFileFromString(pathToMapRutine, mapRutine);
	if (!fileCreated) {
		return 0;
	}

	// Get the block data..
	char *blockData = node_getBlock(numBlock);

	size_t commandSize = strlen(pathToMapRutine) + 9 + strlen(pathToFinalSortedFile) + 3 + strlen(pathToSTDERRFile) + 1;
	char *command = malloc(commandSize);
	snprintf(command, commandSize, "%s | sort >%s 2>%s", pathToMapRutine, pathToFinalSortedFile, pathToSTDERRFile);

	log_info(node_logger, "Executing MAP routine on block number %d. Saving sorted to file in tmp dir as: %s", numBlock, tmpFileName);
	bool result = node_popen_write(command, blockData);
	free(command);
	free(blockData);
	log_info(node_logger, "MAP routine on block number %d to %s executed with status: %s", result ? "OK" : "FAIL");

	return result;
}

bool node_executeReduceRutine(char *reduceRutine, char *tmpFileNameToReduce, char *finalTmpFileName) {
	/************** WRITE ALL FILE PATHS. ******************/
	size_t pathToTmpFileSize = strlen(node_config->tmpDir) + 1 + strlen(finalTmpFileName) + 1;

	char pathToReduceRutine[pathToTmpFileSize + 20];
	char pathToSTDERRFile[pathToTmpFileSize + 20];
	char pathToFinalFile[pathToTmpFileSize];

	strcpy(pathToReduceRutine, node_config->tmpDir);
	strcpy(pathToSTDERRFile, node_config->tmpDir);
	strcpy(pathToFinalFile, node_config->tmpDir);

	strcat(pathToReduceRutine, "/");
	strcat(pathToSTDERRFile, "/");
	strcat(pathToFinalFile, "/");

	strcat(pathToReduceRutine, finalTmpFileName);
	strcat(pathToSTDERRFile, finalTmpFileName);
	strcat(pathToFinalFile, finalTmpFileName);

	strcat(pathToReduceRutine, "_reducerutine");
	strcat(pathToSTDERRFile, "_stderr");
	/************** WRITE ALL FILE PATHS. ******************/

	node_createExecutableFileFromString(pathToReduceRutine, reduceRutine);

	size_t commandSize = 4 + strlen(node_config->tmpDir) + 1 + strlen(tmpFileNameToReduce) + 3 + strlen(pathToReduceRutine) + 2 + strlen(pathToFinalFile) + 3 + strlen(pathToSTDERRFile) + 10;
	char *command = malloc(commandSize);
	snprintf(command, commandSize, "cat %s/%s | sort | %s >%s 2>%s", node_config->tmpDir, tmpFileNameToReduce, pathToReduceRutine, pathToFinalFile, pathToSTDERRFile);
	//TODO ver sort y ver cat.

	log_info(node_logger, "Executing REDUCE routine. Saving final result to file in tmp dir as: %s ", tmpFileNameToReduce, finalTmpFileName);
	bool result = system(command) != -1;
	free(command);
	log_info(node_logger, "REDUCE routine to %s executed with status: %s", result ? "OK" : "FAIL");

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
	close(fd);

	char *fileStr = strdup(fileMap); // TODO memcpy?
	munmap(fileMap, stat.st_size); //TODO Check performance

	return fileStr;
}

char* node_getBlock(uint16_t numBlock) {
	log_info(node_logger, "Getting block number %d", numBlock);

	char *blockStr = malloc(sizeof(char) * BLOCK_SIZE);

	pthread_rwlock_rdlock(&blocks_mutex[numBlock]);
	memcpy(blockStr, binFileMap + (numBlock * BLOCK_SIZE), BLOCK_SIZE);
	pthread_rwlock_unlock(&blocks_mutex[numBlock]);

	return blockStr;
}

void node_setBlock(uint16_t numBlock, char *blockStr) {
	log_info(node_logger, "Setting block number %d", numBlock);

	void *address = binFileMap + (numBlock * BLOCK_SIZE);
	size_t length = strlen(blockStr) + 1;

	pthread_rwlock_wrlock(&blocks_mutex[numBlock]);
	memcpy(address, blockStr, length);
	msync(address, length, MS_SYNC);
	pthread_rwlock_unlock(&blocks_mutex[numBlock]);
}

e_socket_status node_setBlockFromPacket(uint16_t numBlock, int socket) {
	log_info(node_logger, "Setting block number %d from packet", numBlock);

	void *address = binFileMap + (numBlock * BLOCK_SIZE);

	pthread_rwlock_wrlock(&blocks_mutex[numBlock]);
	size_t sBuffer = 0;
	e_socket_status status = socket_recv_packet_to_memory(socket, &address, &sBuffer);
	((char *) address)[sBuffer] = '\0';
	msync(address, sBuffer, MS_SYNC);
	pthread_rwlock_unlock(&blocks_mutex[numBlock]);

	return status;
}

bool node_createExecutableFileFromString(char *pathToFile, char *str) {
	FILE *fp = fopen(pathToFile, "w");
	if (!fp) {
		log_error(node_logger, "Couldn't create executable file %s because open failed", pathToFile);
		return 0;
	}

	if (str) {
		fputs(str, fp);
	}

	int fd = fileno(fp);
	if (!fd) {
		log_error(node_logger, "Couldn't create executable file %s because it couldn't get the fileno", pathToFile);
		fclose(fp);
		return 0;
	}

	// Sets exec mode.
	if (fchmod(fd, 0755)) {
		log_error(node_logger, "Couldn't create executable file %s because it couldn't change the exec mode", pathToFile);
		fclose(fp);
		return 0;
	}

	fflush(fp);
	int result = fclose(fp);
	if (result) {
		log_error(node_logger, "Couldn't create executable file %s because it couldn't close the file", pathToFile);
		return 0;
	}

	return 1;
}

bool node_createTmpFileFromStringList(char *tmpFileName, t_list *stringParts) {
	char pathToTmpFileName[strlen(node_config->tmpDir) + 1 + strlen(tmpFileName) + 1];
	strcpy(pathToTmpFileName, node_config->tmpDir);
	strcat(pathToTmpFileName, "/");
	strcat(pathToTmpFileName, tmpFileName);

	FILE *fp = fopen(pathToTmpFileName, "w");
	if (!fp) {
		return 0;
	}
	bool failed = 0;
	void putContent(char *part) {
		if (part && !failed) {
			if (0 > fputs(part, fp)) {
				failed = 1;
			}
		}
	}
	list_iterate(stringParts, (void*) putContent);
	fclose(fp);
	return !failed;
}

bool node_popen_write(char *command, char *data) {
	FILE *pipe = popen(command, "w");
	if (!pipe) {
		log_error(node_logger, "Couldn't open a pipe to execute the routine");
		return 0;
	}

	int result = fputs(data, pipe);

	if (result < 0) {
		log_error(node_logger, "Couldn't write to the pipe. Result error: %d", result);
		pclose(pipe);
		return 0;
	}

	pclose(pipe);
	return 1;
}

size_t node_getBinFileSize() {
	return node_config->blocksCount * BLOCK_SIZE;
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
		//file =fopen(node_config->binFilePath, "r+");
		fd = open(node_config->binFilePath, O_RDWR);
	}

	if (fd == -1) {
		log_error(node_logger, "Error while trying to open the binFile.");
		return 0;
	}

	size_t binFileSize = node_getBinFileSize();
	if (createFile) {
		if (ftruncate(fd, binFileSize) == -1) {
			log_error(node_logger, "Error while trying to truncate the binFile. errno %d", errno);
			return 0;
		}
	} else {
		// File exists, should check that size matches
		struct stat stat;
		fstat(fd, &stat);
		if (stat.st_size != binFileSize) {
			log_error(node_logger, "You cannot change the size of the bin file if you are not a new node..");
			return 0;
		}
	}
	// ...

	// Map the file
	log_info(node_logger, "Mapping the binFile..");
	binFileMap = mmap(0, binFileSize, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);
	close(fd);

	if (!binFileMap) {
		log_error(node_logger, "Error while trying to map the binFile.");
		return 0;
	}
	// ...

	// Create mutex for blocks
	blocks_mutex = malloc(sizeof(pthread_rwlock_t) * node_config->blocksCount);
	int i;
	for (i = 0; i < node_config->blocksCount; i++) {
		if (pthread_rwlock_init(&blocks_mutex[i], NULL) != 0) {
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

	if (!config) {
		log_error(node_logger, "Config file %s not found!", configFile);
		return 0;
	}

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
		munmap(binFileMap, node_getBinFileSize());
	}

	if (node_config) {
		if (blocks_mutex) {
			int i;
			for (i = 0; i < node_config->blocksCount; i++) {
				pthread_rwlock_destroy(&blocks_mutex[i]);
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
