#include "node.h"

#include "utils/socket.h"
#include "connections/connections.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

int node_initConfig(char* configFile);
void node_waitUntilExit();
bool node_init();
void node_free();

char* node_popen_read(char *command);
bool node_popen_write(char *command, char *data);
bool node_createExecutableFileFromString(char *pathToFile, char *str);

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

	// TODO just testing.
	//node_executeMapRutine("#!/bin/bash \n  cat - | awk -F ',' '{print $2 \";\" $1  \";\" $13 \";\" $3}'\n", 0, "archivo1");
	//return 1;

	connections_initialize();
	log_info(node_logger, "Node Initialized successfully.");
	node_waitUntilExit();
	connections_shutdown();
	node_free();

	return EXIT_SUCCESS;
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

char* node_popen_read(char *command) {
	char line[256];
	FILE *pipe = popen(command, "r");
	if (!pipe) {
		return NULL;
	}

	while (fgets(line, 255, pipe)) {
		// TODO, do we need popen_read?
		printf("%s", line);
	}
	pclose(pipe);

	return NULL; //  TODO
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

bool node_executeMapRutine(char *mapRutine, uint16_t numBlock, char *tmpName) {
	log_info(node_logger, "Executing MAP rutine on block number %d. Saving sorted to file in tmp dir as: %s", numBlock, tmpName);
	// First, get the block data..
	char *blockData = node_getBlock(numBlock);

	size_t commandSize;
	char *command;

	/************** WRITE ALL FILE PATHS. ******************/
	size_t pathToTmpFileSize = strlen(node_config->tmpDir) + 1 + strlen(tmpName) + 1;

	char pathToMapRutine[pathToTmpFileSize + 10];
	char pathToSTDOUTFile[pathToTmpFileSize + 10];
	char pathToSTDERRFile[+pathToTmpFileSize + 10];
	char pathToFinalSortedFile[pathToTmpFileSize];

	strcpy(pathToMapRutine, node_config->tmpDir);
	strcpy(pathToSTDOUTFile, node_config->tmpDir);
	strcpy(pathToSTDERRFile, node_config->tmpDir);
	strcpy(pathToFinalSortedFile, node_config->tmpDir);

	strcat(pathToMapRutine, "/");
	strcat(pathToSTDOUTFile, "/");
	strcat(pathToSTDERRFile, "/");
	strcat(pathToFinalSortedFile, "/");

	strcat(pathToMapRutine, tmpName);
	strcat(pathToSTDOUTFile, tmpName);
	strcat(pathToSTDERRFile, tmpName);
	strcat(pathToFinalSortedFile, tmpName);

	strcat(pathToMapRutine, "_maprutine");
	strcat(pathToSTDOUTFile, "_stdout");
	strcat(pathToSTDERRFile, "_stderr");
	/************** WRITE ALL FILE PATHS. ******************/

	node_createExecutableFileFromString(pathToMapRutine, mapRutine);

	commandSize = strlen(pathToMapRutine) + strlen(pathToSTDOUTFile) + strlen(pathToSTDERRFile) + 6;
	command = malloc(commandSize);
	snprintf(command, commandSize, "%s >%s 2>%s", pathToMapRutine, pathToSTDOUTFile, pathToSTDERRFile);

	bool result = node_popen_write(command, blockData);
	free(command);
	free(blockData);

	if (!result) {
		return 0;
	}

	// SORT the results.. TODO check the stderr ??

	char sortCommand[] = "/usr/bin/sort";
	commandSize = strlen(sortCommand) + strlen(pathToSTDOUTFile) + strlen(pathToFinalSortedFile) + 5;
	command = malloc(commandSize);
	snprintf(command, commandSize, "%s %s > %s", sortCommand, pathToSTDOUTFile, pathToFinalSortedFile);

	if (system(command) == -1) {
		result = 0;
	}

	free(command);

	return result;
}

bool node_executeReduceRutine(char *mapRutine, uint16_t numBlock) {
	log_info(node_logger, "Executing REDUCE rutine on block number %d.", numBlock);
	// TODO
	/*el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
	 El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
	 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
	 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino*/
	return 1;
}

char* node_getFileContent(char *tmpName) {
	/*Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
	 Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
	 getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
	 por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
	 es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.*/
	return "LoVemo";
}

void node_waitUntilExit() {
	pthread_mutex_t keepRunning = PTHREAD_MUTEX_INITIALIZER;

	void intHandler(int dummy) {
		pthread_mutex_unlock(&keepRunning);
	}

	signal(SIGINT, intHandler);
	pthread_mutex_lock(&keepRunning); // Locks it
	pthread_mutex_lock(&keepRunning); // Waits till it is unlocked.
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
	// TODO remove, just testing strcpy(blockStr, ",primero\n,segundo\n,tercero\n,cuarto");
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
	free(blocks_mutex);

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
