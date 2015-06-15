#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <commons/log.h>
#include <pthread.h>
// #include <semaphore.h>

#include "../connections/connections.h"

bool filesystem_canCreateResource(char *resourceName, char *parentId);
char* filesystem_md5(char *str);
t_list* filesystem_getFSFileBlocks(char *route, size_t *fileSize);
char* filesystem_getMD5FromBlocks(t_list *blocks);
bool filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file);
void *filesystem_sendBlockToNode(void *param);
nodeBlockSendOperation_t* filesystem_nodeBlockSendOperation_create(node_t *node, off_t blockIndex, char *block);
void filesystem_nodeBlockSendOperation_free(nodeBlockSendOperation_t* nodeSendBlockOperation);

t_log* filesystem_logger;

void filesystem_initialize() {
	filesystem_logger = log_create("filesystem.log", "MDFS", 0, log_level_from_string("TRACE"));
	mongo_dir_init();
	mongo_file_init();
	mongo_node_init();
}

void filesystem_shutdown() {
	mongo_dir_shutdown();
	mongo_file_shutdown();
	mongo_node_shutdown();
	mongo_shutdown();
	log_destroy(filesystem_logger);
}

bool filesystem_format() {
	log_info(filesystem_logger, "Format FS.");
	if (mongo_dir_deleteAll() && mongo_file_deleteAll()) {
		t_list *nodes = mongo_node_getAll();

		void formatNode(node_t *node) {
			node_setAllBlocksFree(node);
			mongo_node_updateBlocks(node);
		}

		list_iterate(nodes, (void *) formatNode);
		list_destroy_and_destroy_elements(nodes, (void *) node_free);

		return 1;
	}
	return 0;
}

unsigned long filesystem_getFreeSpaceKiloBytes() {
	int freeBlocks = 0;
	t_list *nodes = mongo_node_getAll();

	void sumBlocks(node_t *node) {
		freeBlocks += node_getBlocksFreeCount(node);
	}

	list_iterate(nodes, (void *) sumBlocks);
	list_destroy_and_destroy_elements(nodes, (void *) node_free);

	return freeBlocks * (NODE_BLOCK_SIZE / 1024);
}

dir_t* filesystem_getDirById(char *id) {
	return mongo_dir_getById(id);
}

dir_t* filesystem_getDirByNameInDir(char *dirName, char *parentId) {
	return mongo_dir_getByNameInDir(dirName, parentId);
}

file_t* filesystem_getFileByNameInDir(char *fileName, char *parentId) {
	return mongo_file_getByNameInDir(fileName, parentId);
}

t_list* filesystem_getDirsInDir(char *parentId) {
	return mongo_dir_getByParentId(parentId);
}

t_list* filesystem_getFilesInDir(char *parentId) {
	return mongo_file_getByParentId(parentId);
}

bool filesystem_deleteDirByNameInDir(char *dirName, char *parentId) {
	dir_t *dir = filesystem_getDirByNameInDir(dirName, parentId);
	if (dir) {
		bool r = filesystem_deleteDir(dir);
		dir_free(dir);
		return r;
	}
	return 0;
}

bool filesystem_deleteDir(dir_t *dir) {
	if (dir) {
		void deleteChilds(char *parentId) {
			t_list *childFiles = filesystem_getFilesInDir(parentId);
			list_iterate(childFiles, (void*) filesystem_deleteFile);
			list_destroy_and_destroy_elements(childFiles, (void*) file_free);

			t_list *childDirs = filesystem_getDirsInDir(parentId);
			list_iterate(childDirs, (void*) filesystem_deleteDir);
			list_destroy_and_destroy_elements(childDirs, (void*) dir_free);
		}

		deleteChilds(dir->id);
		mongo_dir_deleteById(dir->id);
		return 1;
	}
	return 0;
}

bool filesystem_deleteFileByNameInDir(char *fileName, char *parentId) {
	file_t *file = mongo_file_getByNameInDir(fileName, parentId);
	if (file) {
		bool r = filesystem_deleteFile(file);
		file_free(file);
		return r;
	}
	return 0;
}

bool filesystem_deleteFile(file_t *file) {
	if (file) {
		// Free nodes space

		void listBlocks(t_list* blockCopies) {
			void listBlockCopy(file_block_t *blockCopy) {
				node_t *node = filesystem_getNodeById(blockCopy->nodeId);
				node_setBlockFree(node, blockCopy->blockIndex);
				mongo_node_updateBlocks(node);
				node_free(node);
			}
			list_iterate(blockCopies, (void *) listBlockCopy);
		}
		list_iterate(file->blocks, (void *) listBlocks);

		mongo_file_deleteById(file->id);
		return 1;
	}
	return 0;
}

void filesystem_moveFile(file_t *file, char *destinationId) {
	mongo_file_updateParentId(file->id, destinationId);
}

void filesystem_moveDir(dir_t *dir, char *destinationId) {
	mongo_dir_updateParentId(dir->id, destinationId);
}

int filesystem_copyFileFromFS(char *route, file_t *file) {
	if (!filesystem_canCreateResource(file->name, file->parentId)) {
		return -1;
	}

	size_t fileSize;

	t_list *blocks = filesystem_getFSFileBlocks(route, &fileSize);
	file->size = fileSize;

	// TESTING ONLY printf("%s\n", filesystem_getMD5FromBlocks(blocks));
	if (!filesystem_distributeBlocksToNodes(blocks, file)) {
		list_destroy_and_destroy_elements(blocks, free);
		return -2;
	}
	list_destroy_and_destroy_elements(blocks, free);

	if (!mongo_file_save(file)) {
		// If for some reason, the file could not be saved in the db, then should destroy the blocks that were used for that file.
		filesystem_deleteFile(file);
		return -3;
	}

	return 1;
}

bool filesystem_addDir(dir_t *dir) {
	if (!filesystem_canCreateResource(dir->name, dir->parentId)) {
		return 0;
	}

	return mongo_dir_save(dir);
}

node_t* filesystem_getNodeById(char *nodeId) {
	return mongo_node_getById(nodeId);
}

void filesystem_nodeIsDown(char *nodeId) {
	node_t *node = filesystem_getNodeById(nodeId);
	if (node) {
		t_list *files = mongo_file_getFilesThatHaveNode(node->id);
		void listFile(file_t *file) {
			// TODO..
			printf("TODO file used: %s\n", file->name);
		}
		list_iterate(files, (void*) listFile);
		list_destroy_and_destroy_elements(files, (void *) file_free);

		node_free(node);
	}
}

node_t* filesystem_addNode(char *nodeId, uint16_t blocksCount) {
	node_t *node = filesystem_getNodeById(nodeId);
	if (node) {
		t_list *files = mongo_file_getFilesThatHaveNode(node->id);
		// TODO: marcar los bloques del archivo como disponibles o que?.
		list_destroy_and_destroy_elements(files, (void *) file_free);
	} else {
		// Nuevo nodo
		// TODO
		log_info(filesystem_logger, "New NODE connected. Name: %s . blocksCount %d", nodeId, blocksCount);
		node = node_create(blocksCount);
		node->id = strdup(nodeId);
		mongo_node_save(node);
	}
	return node;
}

char* filesystem_md5sum(file_t* file) {
	// TODO
	t_list *blocksData = list_create();
	void listBlocks(t_list* blockCopies) {
		void listBlockCopy(file_block_t *blockCopy) {
			// TODO, move to threads and join later after iterate.
			char *block = connections_getBlockFromNode(blockCopy);
			if (!block) {
				// TODO, aca deberia ir a buscar en el siguiente nodo que tenga este mismo bloque.
				printf("BLOCK ES NULL :O");
			}
			list_add(blocksData, block);

		}
		list_iterate(blockCopies, (void *) listBlockCopy);
	}
	list_iterate(file->blocks, (void *) listBlocks);

	char *md5str = filesystem_getMD5FromBlocks(blocksData);
	list_destroy_and_destroy_elements(blocksData, free);
	return md5str;
}

// PRIVATE

bool filesystem_canCreateResource(char *resourceName, char *parentId) {
	dir_t *dir = filesystem_getDirByNameInDir(resourceName, parentId);
	if (dir) {
		dir_free(dir);
		return 0;
	}

	file_t *file = filesystem_getFileByNameInDir(resourceName, parentId);
	if (file) {
		file_free(file);
		return 0;
	}

	return 1;
}

char* filesystem_md5(char *str) {
	char FILE_NAME[] = "/tmp/mdfs_md5tmp";

	FILE *fp = fopen(FILE_NAME, "w");
	fputs(str, fp);
	fclose(fp);

	FILE *md5pipe = NULL;
	size_t size = 7 + strlen(FILE_NAME) + 1;
	char *command = malloc(size);
	snprintf(command, size, "md5sum %s", FILE_NAME);

	md5pipe = popen(command, "r");

	if (md5pipe != NULL) {
		char *buffer = malloc(sizeof(char) * 33);
		fread(buffer, 1, 32, md5pipe);
		buffer[32] = '\0';

		pclose(md5pipe);
		free(command);

		return buffer;
	} else {
		free(command);
		return NULL;
	}
}

t_list* filesystem_getFSFileBlocks(char *route, size_t *fileSize) {
	struct stat stat;
	int fd = open(route, O_RDONLY);

	//Get the size of the file.
	fstat(fd, &stat);
	*fileSize = stat.st_size;

	char *fileStr = (char *) mmap(0, *fileSize, PROT_READ, MAP_PRIVATE, fd, 0);

	char *buffer;
	char *startBlock;
	t_list *blocks = list_create();

	int j;
	int i = 0;
	int finished = 0;

	void addBlock(int blockLength) {
		buffer = malloc(sizeof(char) * NODE_BLOCK_SIZE); // TODO. poner blockLength (+1) tal vez? testear bien. con md5
		strncpy(buffer, startBlock, blockLength);
		buffer[blockLength] = '\0';
		list_add(blocks, buffer);
		// TODO (es necesario??), fill the rest with \0
	}

	while (!finished) {
		startBlock = fileStr + i;

		// It's the last block.
		if (i + NODE_BLOCK_SIZE > *fileSize) {
			addBlock(*fileSize - i);
			finished = 1;
		} else {
			for (j = i + NODE_BLOCK_SIZE - 1; j > i; j--) {
				if (fileStr[j] == '\n') {
					addBlock(j - i + 1);
					i = j + 1;
					break;
				}
			} // TODO, que pasa si la linea supera el maximo tamaño del buffer?
		}
	}

	munmap(fileStr, *fileSize);

	return blocks;
}

char* filesystem_getMD5FromBlocks(t_list *blocks) {
	char *fileBuffer = strdup("");
	void concatBuffers(char *buffer) {
		fileBuffer = realloc(fileBuffer, sizeof(char) * (strlen(fileBuffer) + strlen(buffer) + 1));
		strcat(fileBuffer, buffer);
	}
	list_iterate(blocks, (void *) concatBuffers);

	char *md5str = filesystem_md5(fileBuffer);

	free(fileBuffer);
	return md5str;
}

bool filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file) {
	bool success = 1;

	t_list *nodes = mongo_node_getAll();
	t_list *sendOperations = list_create();

	bool nodeComparator(node_t *node, node_t *node2) {
		return node_getBlocksFreeCount(node) > node_getBlocksFreeCount(node2);
	}

	void createSendOperations(char *block) {
		if (!success) {
			return;
		}
		t_list *blockCopies = list_create();
		list_add(file->blocks, blockCopies);

		// Sort to get the node that has more free space.
		list_sort(nodes, (void *) nodeComparator);

		int i;
		for (i = 0; i < FILESYSTEM_BLOCK_COPIES; i++) {
			if (i < list_size(nodes)) {
				node_t *selectedNode = list_get(nodes, i);
				off_t firstBlockFreeIndex = node_getFirstFreeBlock(selectedNode);

				if (firstBlockFreeIndex != -1) {
					// El bloque del nodo se setea como usado para seguir la planificacion pero NO se guarda. Solo se va a guardar si te iteran las operaciones.
					node_setBlockUsed(selectedNode, firstBlockFreeIndex);
					list_add(sendOperations, filesystem_nodeBlockSendOperation_create(selectedNode, firstBlockFreeIndex, block));

					file_block_t *blockCopy = file_block_create();
					blockCopy->nodeId = strdup(selectedNode->id);
					blockCopy->blockIndex = firstBlockFreeIndex;
					list_add(blockCopies, blockCopy);
				} else {
					success = 0;
					log_error(filesystem_logger, "Couldn't find a free block to store the block");
					return;
				}
			} else {
				success = 0;
				log_error(filesystem_logger, "There are less free nodes than copies to be done.");
				return;
			}
		}
	}

	list_iterate(blocks, (void *) createSendOperations);

	if (success) {
		pthread_t threads[list_size(sendOperations)];
		int count = 0;

		void runOperations(nodeBlockSendOperation_t *sendOperation) {
			// Aca es donde actualizo el bloque usado porque realmente se lo voy a mandar, sino no se actualiza y se destruye sin guardar.
			mongo_node_updateBlocks(sendOperation->node);
			// Create threads and do join later.
			if (pthread_create(&(threads[count]), NULL, (void *) filesystem_sendBlockToNode, (void*) sendOperation)) {
				return; // -1; // TODO error
			}
			count++;
		}

		list_iterate(sendOperations, (void *) runOperations);

		int i;
		for (i = 0; i < count; i++) {
			// TODO Por ahora solo esta hecho porque sino pierdo referencias en el medio.. una cagada.
			pthread_join(threads[i], NULL);
		}
	}

	// destoy operations and nodes..
	list_destroy(sendOperations); // The items are destroyed after usage.
	list_destroy_and_destroy_elements(nodes, (void *) node_free);

	return success;
}

void *filesystem_sendBlockToNode(void *param) {

	nodeBlockSendOperation_t* sendOperation = (nodeBlockSendOperation_t*) param;

	// TODO mandar a node.
	log_info(filesystem_logger, "Sending block to node %s , blockIndex %d", sendOperation->node->id, sendOperation->blockIndex);
	connections_sendBlockToNode(sendOperation);

	filesystem_nodeBlockSendOperation_free(sendOperation);

	return NULL;
}

nodeBlockSendOperation_t* filesystem_nodeBlockSendOperation_create(node_t *node, off_t blockIndex, char *block) {
	nodeBlockSendOperation_t *nodeSendBlockOperation = malloc(sizeof(nodeBlockSendOperation_t));
	nodeSendBlockOperation->node = node;
	nodeSendBlockOperation->blockIndex = blockIndex;
	nodeSendBlockOperation->block = block;
	return nodeSendBlockOperation;
}

void filesystem_nodeBlockSendOperation_free(nodeBlockSendOperation_t* nodeSendBlockOperation) {
	// Point it to null because they are free'd by others.
	nodeSendBlockOperation->node = NULL;
	nodeSendBlockOperation->block = NULL;
	free(nodeSendBlockOperation);
}
/*
 * MUTEX EXAMPLE
 * pthread_mutex_t lock;
 void a() {
 if (pthread_mutex_init(&lock, NULL) != 0) {
 // TODO error.
 return;
 }

 pthread_mutex_lock(&lock);

 // REGION CRITICA.

 pthread_mutex_unlock(&lock);

 pthread_mutex_destroy(&lock);
 }
 */
