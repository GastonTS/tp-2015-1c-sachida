#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <commons/log.h>

bool filesystem_canCreateResource(char *resourceName, char *parentId);
char* filesystem_md5(char *str);
t_list* filesystem_getFSFileBlocks(char *route, size_t *fileSize);
char* filesystem_getMD5FromBlocks(t_list *blocks);
void filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file);
void filesystem_sendBlockToNode(node_t *node, off_t blockIndex, char *block);

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

	void deleteChilds(char *parentId) {
		void deleteFile(file_t *file) {
			mongo_file_deleteById(file->id);
		}
		t_list *childFiles = filesystem_getFilesInDir(parentId);
		list_iterate(childFiles, (void*) deleteFile);
		list_destroy_and_destroy_elements(childFiles, (void*) file_free);

		void deleteDir(dir_t *dir) {
			deleteChilds(dir->id);
			mongo_dir_deleteById(dir->id);
		}
		t_list *childDirs = filesystem_getDirsInDir(parentId);
		list_iterate(childDirs, (void*) deleteDir);
		list_destroy_and_destroy_elements(childDirs, (void*) dir_free);
	}

	dir_t *dir = filesystem_getDirByNameInDir(dirName, parentId);
	if (dir) {
		deleteChilds(dir->id);
		bool r = mongo_dir_deleteById(dir->id);
		dir_free(dir);
		return r;
	} else {
		return 0;
	}
}

bool filesystem_deleteFileByNameInDir(char *fileName, char *parentId) {
	return mongo_file_deleteFileByNameInDir(fileName, parentId);
}

void filesystem_moveFile(file_t *file, char *destinationId) {
	mongo_file_updateParentId(file->id, destinationId);
}

void filesystem_moveDir(dir_t *dir, char *destinationId) {
	mongo_dir_updateParentId(dir->id, destinationId);
}

bool filesystem_copyFileFromFS(char *route, file_t *file) {
	if (!filesystem_canCreateResource(file->name, file->parentId)) {
		return 0;
	}

	size_t fileSize;

	t_list *blocks = filesystem_getFSFileBlocks(route, &fileSize);
	file->size = fileSize;

	// TESTING ONLY printf("%s\n", filesystem_getMD5FromBlocks(blocks));
	filesystem_distributeBlocksToNodes(blocks, file);
	list_destroy_and_destroy_elements(blocks, free);

	return !mongo_file_save(file);
}

bool filesystem_addDir(dir_t *dir) {
	if (!filesystem_canCreateResource(dir->name, dir->parentId)) {
		return 0;
	}

	return !mongo_dir_save(dir);
}

node_t* filesystem_getNodeByName(char *nodeName) {
	return mongo_node_getByName(nodeName);
}

void filesystem_nodeIsDown(char *nodeName) {
	node_t *node = filesystem_getNodeByName(nodeName);
	if (node) {
		t_list *files = mongo_file_getFilesThatHaveNode(node->id);
		void listFile(file_t *file) {
			// TODO..
			printf("%s\n", file->name);
		}
		list_iterate(files, (void*) listFile);
		list_destroy_and_destroy_elements(files, (void *) file_free);

		node_free(node);
	}
}

char* filesystem_md5sum(file_t* file) {
	// TODO
	return filesystem_md5("asdasd");
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
		buffer = malloc(sizeof(char) * NODE_BLOCK_SIZE);
		strncpy(buffer, startBlock, blockLength);
		buffer[blockLength] = '\0';
		list_add(blocks, buffer);
		// TODO (es necesario??), fill the rest with \0 .. and other stuff.input[strlen(input) - 1] = '\0';
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

void filesystem_distributeBlocksToNodes(t_list *blocks, file_t *file) {
	t_list *nodes = mongo_node_getAll();

	bool nodeComparator(node_t *node, node_t *node2) {
		return node_getBlocksFreeCount(node) > node_getBlocksFreeCount(node2);
	}

	void sendBlockToNodes(char *block) {
		t_list *blockCopies = list_create();
		list_add(file->blocks, blockCopies);

		// Sort to get the node that has more free space.
		list_sort(nodes, (void *) nodeComparator);
		int i;

		for (i = 0; i < FILESYSTEM_BLOCK_COPIES; i++) {
			if (i < list_size(nodes)) {
				node_t *selectedNode = list_get(nodes, i);
				off_t firstBlockFreeIndex = node_getFirstFreeBlock(selectedNode);

				// TESTING node_printBlocksStatus(selectedNode);
				if (firstBlockFreeIndex == -1) {
					// TODO ROLLBACK (or avoid), no hay mas nodos disponibles
					log_error(filesystem_logger, "Couldn't find a free block to store the block");
				} else {
					filesystem_sendBlockToNode(selectedNode, firstBlockFreeIndex, block);
					node_setBlockUsed(selectedNode, firstBlockFreeIndex);
					mongo_node_updateBlocks(selectedNode);

					file_block_t *blockCopy = file_block_create();
					strcpy(blockCopy->nodeId, selectedNode->id);
					blockCopy->blockIndex = firstBlockFreeIndex;
					list_add(blockCopies, blockCopy);
				}
			} else {
				//TODO ROLLBACK (or avoid) no hay tanta cantidad de nodos disponibles como de copias necesarias.
				log_error(filesystem_logger, "There are less nodes than copies to be done.");
			}
		}
	}

	list_iterate(blocks, (void *) sendBlockToNodes);
	list_destroy_and_destroy_elements(nodes, (void *) node_free);
}

void filesystem_sendBlockToNode(node_t *node, off_t blockIndex, char *block) {
	// TODO
	log_info(filesystem_logger, "Sending block to node %s (%s), blockIndex %d\n", node->name, node->id, blockIndex);
}
