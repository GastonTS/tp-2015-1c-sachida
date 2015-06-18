#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <pthread.h>
#include <commons/string.h>
// #include <semaphore.h>

#include "../connections/connections_node.h"

void filesystem_createLocalFileFromString(char *pathToFile, char *str);
bool filesystem_canCreateResource(char *resourceName, char *parentId);
char* filesystem_md5(char *str);
t_list* filesystem_getFSFileBlocks(char *route, size_t *fileSize);
char* filesystem_getMD5FromBlocks(t_list *blocks);
bool filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file);
void *filesystem_sendBlockToNode(void *param);
nodeBlockSendOperation_t* filesystem_nodeBlockSendOperation_create(node_t *node, off_t blockIndex, char *block);
void filesystem_nodeBlockSendOperation_free(nodeBlockSendOperation_t* nodeSendBlockOperation);
bool filesystem_nodeComparatorByBlocksFree(node_t *node, node_t *node2);
bool filesystem_isRootDirId(char *id);
bool filesystem_isRootDir(dir_t *dir);

t_log* mdfs_logger;

void filesystem_initialize() {
	mdfs_logger = log_create("filesystem.log", "MDFS", 0, log_level_from_string("TRACE"));
	mongo_dir_init();
	mongo_file_init();
	mongo_node_init();
}

void filesystem_shutdown() {
	mongo_dir_shutdown();
	mongo_file_shutdown();
	mongo_node_shutdown();
	mongo_shutdown();
	log_destroy(mdfs_logger);
}

bool filesystem_format() {
	log_info(mdfs_logger, "Format FS.");
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
		return mongo_dir_deleteById(dir->id);
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

		return mongo_file_deleteById(file->id);
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
	if (!blocks) {
		return -2;
	}

	file->size = fileSize;

	// TESTING ONLY printf("%s\n", filesystem_getMD5FromBlocks(blocks));
	if (!filesystem_distributeBlocksToNodes(blocks, file)) {
		list_destroy_and_destroy_elements(blocks, free);
		return -3;
	}
	list_destroy_and_destroy_elements(blocks, free);

	if (!mongo_file_save(file)) {
		// If for some reason, the file could not be saved in the db, then should destroy the blocks that were used for that file.
		filesystem_deleteFile(file);
		return -4;
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

void filesystem_addNode(char *nodeId, uint16_t blocksCount, bool isNewNode) {
	node_t *node = filesystem_getNodeById(nodeId);
	log_info(mdfs_logger, "NODE (new: %s) connected. Name: %s . blocksCount %d", isNewNode ? "true" : "false", nodeId, blocksCount);
	if (node) {
		t_list *files = mongo_file_getFilesThatHaveNode(node->id);
		// TODO: marcar los bloques del archivo como disponibles o que?. (chequear i es newnode o no)
		list_destroy_and_destroy_elements(files, (void *) file_free);
	} else {
		// Nuevo nodo
		// TODO
		node = node_create(blocksCount);
		node->id = strdup(nodeId);
		mongo_node_save(node);
	}
	node_free(node);
}

char* filesystem_md5sum(file_t* file) {
	// TODO, mover a thread?..
	//pthread_t getBlockThread[list_size(file->blocks)];
	//int thCount = 0;
	bool found = 0;
	bool isComplete = 1;

	t_list *blocksData = list_create();
	void listBlocks(t_list* blockCopies) {
		if (!isComplete) {
			return;
		}
		void listBlockCopy(file_block_t *blockCopy) {
			if (!found) {
				char *block = connections_node_getBlock(blockCopy);
				if (block) {
					found = 1;
					list_add(blocksData, block);
				}
			}
		}
		found = 0;
		list_iterate(blockCopies, (void *) listBlockCopy);
		if (!found) {
			isComplete = 0;
		}
	}
	list_iterate(file->blocks, (void *) listBlocks);

	if (!isComplete) {
		return NULL;
	}
	char *md5str = filesystem_getMD5FromBlocks(blocksData);
	list_destroy_and_destroy_elements(blocksData, free);
	return md5str;
}

/*
 * Resolves a path for a file like: folder/../folder/folder2/../../FILE
 */
file_t* filesystem_resolveFilePath(char *path, char *startingDirId, char *startingPath) {
	int i;
	int lastSlashPos = -1;
	char *fileName;

	for (i = strlen(path) - 1; i >= 0; i--) {
		if (path[i] == '/') {
			lastSlashPos = i;
			break;
		}
	}

	dir_t *fileDir;
	if (lastSlashPos != -1) {
		char *fileDirPath = string_substring_until(path, lastSlashPos + 1);
		fileDir = filesystem_resolveDirPath(fileDirPath, startingDirId, startingPath, NULL);
		free(fileDirPath);
		fileName = string_substring_from(path, lastSlashPos + 1);
	} else {
		if (filesystem_isRootDirId(startingDirId)) {
			fileDir = dir_create();
			strcpy(fileDir->id, ROOT_DIR_ID);
		} else {
			fileDir = filesystem_getDirById(startingDirId);
		}
		fileName = strdup(path);
	}

	if (!fileDir) {
		free(fileName);
		return NULL;
	}

	file_t *file = filesystem_getFileByNameInDir(fileName, fileDir->id);
	dir_free(fileDir);
	free(fileName);

	return file;
}

/*
 * Resolves a path like: folder/../folder/folder2/../.. and sets the fullPath in fullPath
 * The string fullPath is allocated using malloc.
 */
dir_t* filesystem_resolveDirPath(char *path, char *startingDirId, char *startingPath, char **fullPath) {
	char **dirNames;
	char *dirName;
	int i = 0;

	dir_t *currentDir;
	dir_t *newDir;

	char newFullPath[1024];

	if (path[0] == '/') {
		strcpy(newFullPath, "/");
		dirNames = string_split(path + 1, "/"); // Avoid the first slash
	} else {
		strcpy(newFullPath, startingPath);
		dirNames = string_split(path, "/");
	}

	if (path[0] == '/' || filesystem_isRootDirId(startingDirId)) {
		currentDir = dir_create();
		strcpy(currentDir->id, ROOT_DIR_ID);
	} else {
		currentDir = filesystem_getDirById(startingDirId);
	}

	if (!currentDir) {
		freeSplits(dirNames);
		return NULL;
	}

	while (dirNames[i]) {
		dirName = dirNames[i];

		if (strcmp(dirName, "") != 0) {
			if (strcmp(dirName, "..") == 0) {
				if (!filesystem_isRootDir(currentDir)) {
					// Removes the last folder in the prompt
					newFullPath[string_length(newFullPath) - string_length(currentDir->name) - 1] = '\0';

					if (filesystem_isRootDirId(currentDir->parentId)) {
						dir_free(currentDir);
						currentDir = dir_create();
						strcpy(currentDir->id, ROOT_DIR_ID);
					} else {
						newDir = filesystem_getDirById(currentDir->parentId);
						dir_free(currentDir);
						currentDir = newDir;
					}

					if (filesystem_isRootDir(currentDir)) {
						strcat(newFullPath, "/");
					}
				}
			} else if (strcmp(dirName, ".") == 0) {
				// Do nothing, keep the same dir.
			} else {
				newDir = filesystem_getDirByNameInDir(dirName, currentDir->id);
				dir_free(currentDir);
				currentDir = newDir;

				if (currentDir) {
					if (!filesystem_isRootDirId(currentDir->parentId)) {
						strcat(newFullPath, "/");
					}
					strcat(newFullPath, currentDir->name);
				} else {
					freeSplits(dirNames);
					return NULL;
				}
			}
		}
		i++;
	}

	if (fullPath) {
		*fullPath = strdup(newFullPath);
	}

	freeSplits(dirNames);

	return currentDir;
}

/*
 * Saves the contents of a block of the file selected
 * Return codes:
 * 	-1 -> Invalid block index
 * 	-2 -> All nodes are down for this block
 * 	-3 -> Passed null to file
 * 	 1 -> Ok!
 */

int filesystem_saveFileBlockToFile(file_t *file, uint16_t blockIndex, char *pathToFile) {
	if (file) {
		if (blockIndex >= list_size(file->blocks)) {
			return -1;
		}
		t_list *blockCopies = list_get(file->blocks, blockIndex);

		int found = 0;
		void listBlockCopy(file_block_t *blockCopy) {
			if (!found) {
				char *block = connections_node_getBlock(blockCopy);
				if (block) {
					found = 1;
					filesystem_createLocalFileFromString(pathToFile, block);
					free(block);
				}
			}
		}
		list_iterate(blockCopies, (void*) listBlockCopy);

		if (!found) {
			return -2;
		}

		return 1;
	}
	return -3;
}

/*
 * Makes a new copy for the block of the file selected
 * Return codes:
 * 	-1 -> Invalid block index
 * 	-2 -> All nodes are down for this block
 * 	-3 -> No free nodes to do the copy
 * 	-4 -> Passed null to file
 * 	 1 -> Ok!
 */
int filesystem_makeNewFileBlockCopy(file_t *file, uint16_t blockIndex) {
	if (file) {
		if (blockIndex >= list_size(file->blocks)) {
			return -1;
		}
		t_list *blockCopies = list_get(file->blocks, blockIndex);

		char *block = NULL;
		void listBlockCopy(file_block_t *blockCopy) {
			if (!block) {
				block = connections_node_getBlock(blockCopy);
			}
		}
		list_iterate(blockCopies, (void*) listBlockCopy);

		if (!block) {
			return -2;
		}

		t_list *nodes = mongo_node_getAll();
		list_sort(nodes, (void *) filesystem_nodeComparatorByBlocksFree);

		node_t *selectedNode = NULL;
		void findCandidateNode(node_t *node) {
			if (!selectedNode) {
				int found = 0;
				void listBlockCopy(file_block_t *blockCopy) {
					if (strcmp(blockCopy->nodeId, node->id) == 0) {
						found = 1;
					}
				}
				list_iterate(blockCopies, (void*) listBlockCopy);
				if (!found) {
					selectedNode = node;
				}
			}
		}
		list_iterate(nodes, (void *) findCandidateNode);

		if (selectedNode) {
			off_t firstBlockFreeIndex = node_getFirstFreeBlock(selectedNode);

			if (firstBlockFreeIndex != -1) {
				nodeBlockSendOperation_t *nodeBlockSendOperation = filesystem_nodeBlockSendOperation_create(selectedNode, firstBlockFreeIndex, block);
				filesystem_sendBlockToNode((void *) nodeBlockSendOperation);

				node_setBlockUsed(selectedNode, firstBlockFreeIndex);
				mongo_node_updateBlocks(selectedNode);

				file_block_t *blockCopy = file_block_create();
				blockCopy->nodeId = strdup(selectedNode->id);
				blockCopy->blockIndex = firstBlockFreeIndex;
				list_add(blockCopies, blockCopy);
				mongo_file_addBlockCopyToFile(file->id, blockIndex, blockCopy);

				list_destroy_and_destroy_elements(nodes, (void *) node_free);
				return 1;
			} else {
				list_destroy_and_destroy_elements(nodes, (void *) node_free);
				log_error(mdfs_logger, "There are no nodes to do the copy!");
				return -3;
			}
		} else {
			list_destroy_and_destroy_elements(nodes, (void *) node_free);
			log_error(mdfs_logger, "There are no nodes to do the copy!");
			return -3;
		}
	}
	return -4;
}

/*
 * Deletes a copy for the block of the file selected
 * Return codes:
 * 	-1 -> Invalid block index
 * 	-2 -> Invalid copy index
 * 	-3 -> Passed null to file
 * 	 1 -> Ok!
 */
int filesystem_deleteFileBlockCopy(file_t *file, uint16_t blockIndex, uint16_t copyIndex) {
	if (file) {
		if (blockIndex >= list_size(file->blocks)) {
			return -1;
		}
		t_list *blockCopies = list_get(file->blocks, blockIndex);

		if (copyIndex >= list_size(blockCopies)) {
			return -2;
		}
		file_block_t *blockCopy = list_get(blockCopies, copyIndex);

		mongo_file_deleteBlockCopy(file->id, blockIndex, blockCopy);

		node_t *node = filesystem_getNodeById(blockCopy->nodeId);
		if (node) {
			node_setBlockFree(node, blockCopy->blockIndex);
			mongo_node_updateBlocks(node);
			node_free(node);
		}
		return 1;
	}
	return -3;
}

// PRIVATE

bool filesystem_isRootDirId(char *id) {
	return string_equals_ignore_case(id, ROOT_DIR_ID);
}

bool filesystem_isRootDir(dir_t *dir) {
	return filesystem_isRootDirId(dir->id);
}

void filesystem_createLocalFileFromString(char *pathToFile, char *str) {
	FILE *fp = fopen(pathToFile, "w");
	fputs(str, fp);
	fclose(fp);
}

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
	char FILE_NAME[] = "/tmp/MDFS_md5tmp";

	filesystem_createLocalFileFromString(FILE_NAME, str);

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
	int fd = open(route, O_RDONLY);
	if (fd == -1) {
		return NULL;
	}

	//Get the size of the file.
	struct stat stat;
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
		buffer = malloc(blockLength + 1);
		strncpy(buffer, startBlock, blockLength);
		buffer[blockLength] = '\0';
		list_add(blocks, buffer);
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

bool filesystem_nodeComparatorByBlocksFree(node_t *node, node_t *node2) {
	return node_getBlocksFreeCount(node) > node_getBlocksFreeCount(node2);
}

bool filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file) {
	bool success = 1;

	t_list *nodes = mongo_node_getAll();
	t_list *sendOperations = list_create();

	void createSendOperations(char *block) {
		if (!success) {
			return;
		}
		t_list *blockCopies = list_create();
		list_add(file->blocks, blockCopies);

		// Sort to get the node that has more free space.
		list_sort(nodes, (void *) filesystem_nodeComparatorByBlocksFree);

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
					log_error(mdfs_logger, "Couldn't find a free block to store the block");
					return;
				}
			} else {
				success = 0;
				log_error(mdfs_logger, "There are less free nodes than copies to be done.");
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
				log_error(mdfs_logger, "Error while trying to create new thread: filesystem_sendBlockToNode");
			}
			count++;
		}

		list_iterate(sendOperations, (void *) runOperations);

		int i;
		for (i = 0; i < count; i++) {
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

	log_info(mdfs_logger, "Sending block to node %s , blockIndex %d", sendOperation->node->id, sendOperation->blockIndex);
	// TODO what happens if fails? :O. connections_sendBlockToNode(sendOperation);

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
 log_error(mdfs_logger, "Error while trying to create new mutex");
 return;
 }

 pthread_mutex_lock(&lock);

 // REGION CRITICA.

 pthread_mutex_unlock(&lock);

 pthread_mutex_destroy(&lock);
 }
 */
