#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>

#include "console.h"
#include "../filesystem/filesystem.h"

#define ANSI_COLOR_RED         "\033[31m"
#define ANSI_COLOR_GREEN       "\033[32m"
#define ANSI_COLOR_YELLOW      "\033[33m"
#define ANSI_COLOR_BLUE        "\033[34m"
#define ANSI_COLOR_MAGENTA     "\033[35m"
#define ANSI_COLOR_CYAN        "\033[36m"
#define ANSI_COLOR_BOLDBLACK   "\033[1m\033[30m"
#define ANSI_COLOR_BOLDRED     "\033[1m\033[31m"
#define ANSI_COLOR_BOLDGREEN   "\033[1m\033[32m"
#define ANSI_COLOR_BOLDYELLOW  "\033[1m\033[33m"
#define ANSI_COLOR_BOLDBLUE    "\033[1m\033[34m"
#define ANSI_COLOR_BOLDMAGENTA "\033[1m\033[35m"
#define ANSI_COLOR_BOLDCYAN    "\033[1m\033[36m"
#define ANSI_COLOR_BOLDWHITE   "\033[1m\033[37m"
#define ANSI_COLOR_RESET       "\033[0m"

void readCommand(char *command);
int isNull(char *parameter);
int getIntFromString(char *string);

void format();
void diskFree();

void deleteResource(char **parameters);
void moveResource(char *resource, char *destination);

void copyFile(char **parameters);
void makeDir(char *dirName);
void changeDir(char *dirName);
void listResources();

void md5sum(char *filePath);
void saveBlockContents(char **parameters);
void deleteBlock(char **parameters);
void copyBlock(char **parameters);

void enableNode(char *node);
void disableNode(char *nodeName);
void printNodeStatus(char *nodeName);
void printFileBlocks(char *filePath);

void help();

char currentDirPrompt[1024];
char currentDirId[ID_SIZE];

void console_start() {
	char **parameters;
	char command[512];
	int exit = 0;

	strcpy(currentDirPrompt, "/");
	strcpy(currentDirId, ROOT_DIR_ID);

	do {
		printf("%s > ", currentDirPrompt);
		readCommand(command);
		// El trim de las commons tira error a veces.. string_trim(&command);

		// Ignore empty enter
		if (command[0] != '\0') {
			parameters = string_split(command, " ");

			if (string_equals_ignore_case(parameters[0], "format")) {
				format();
			} else if (string_equals_ignore_case(parameters[0], "df")) {
				diskFree();
			} else if (string_equals_ignore_case(parameters[0], "rm")) {
				deleteResource(parameters);
			} else if (string_equals_ignore_case(parameters[0], "mv")) {
				moveResource(parameters[1], parameters[2]);
			} else if (string_equals_ignore_case(parameters[0], "mkdir")) {
				makeDir(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "cd")) {
				changeDir(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "ll")) {
				listResources();
			} else if (string_equals_ignore_case(parameters[0], "md5sum")) {
				md5sum(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "cp")) {
				copyFile(parameters);
			} else if (string_equals_ignore_case(parameters[0], "blocks")) {
				printFileBlocks(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "catb")) {
				saveBlockContents(parameters);
			} else if (string_equals_ignore_case(parameters[0], "cpb")) {
				copyBlock(parameters);
			} else if (string_equals_ignore_case(parameters[0], "rmb")) {
				deleteBlock(parameters);
			} else if (string_equals_ignore_case(parameters[0], "nodestat")) {
				printNodeStatus(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "enablen")) {
				enableNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "disablen")) {
				disableNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "help")) {
				help();
			} else if (string_equals_ignore_case(parameters[0], "exit")) {
				exit = 1;
			} else if (string_equals_ignore_case(parameters[0], "\n")) {
				// ignore enter
			} else {
				printf("Invalid command \n");
			}
			freeSplits(parameters);
		}
	} while (!exit);

	printf("bye\n");
}

void readCommand(char *input) {
	char buffer[512];

	fgets(buffer, sizeof(buffer), stdin);

	strcpy(input, buffer);

	input[strlen(input) - 1] = '\0'; // Removes the \n
}

int isNull(char *parameter) {
	if (parameter == NULL) {
		printf("You are missing one or more parameters.. \n");
		return 1;
	}
	return 0;
}

// FUNCTIONS FOR EACH COMMAND ..

void format() {
	if (filesystem_format()) {
		strcpy(currentDirId, ROOT_DIR_ID);
		strcpy(currentDirPrompt, "/");
	} else {
		printf("An unexpected error occured\n");
	}
}

void diskFree() {
	unsigned long kbFree = filesystem_getFreeSpaceKiloBytes();
	printf("MDFS\t\t%lu kb = %lu mb\n", kbFree, kbFree / 1024);
}

void deleteResource(char **parameters) {
	if (!isNull(parameters[1])) {
		if (string_equals_ignore_case(parameters[1], "-r")) {
			dir_t *dir = filesystem_resolveDirPath(parameters[2], currentDirId, currentDirPrompt, NULL);
			if (dir) {
				filesystem_deleteDir(dir);
				dir_free(dir);
			} else {
				printf("Cannot remove '%s': No such directory.\n", parameters[2]);
			}
		} else {
			file_t *file = filesystem_resolveFilePath(parameters[1], currentDirId, currentDirPrompt);
			if (file) {
				filesystem_deleteFile(file);
				file_free(file);
			} else {
				printf("Cannot remove '%s': No such file.\n", parameters[1]);
			}
		}
	}
}

void moveResource(char *resource, char *destination) {
	if (!isNull(resource) && !isNull(destination)) {

		dir_t *destinationDir = filesystem_resolveDirPath(destination, currentDirId, currentDirPrompt, NULL);
		if (!destinationDir) {
			printf("Cannot move: dir '%s' not found", destination);
			return;
		}

		dir_t *dirToMove = filesystem_resolveDirPath(resource, currentDirId, currentDirPrompt, NULL);
		if (dirToMove) {
			filesystem_moveDir(dirToMove, destinationDir->id);
			dir_free(dirToMove);
		} else {
			// If couldn't find a dir, then try to find a file:
			file_t *fileToMove = filesystem_resolveFilePath(resource, currentDirId, currentDirPrompt);
			if (fileToMove) {
				filesystem_moveFile(fileToMove, destinationDir->id);
				file_free(fileToMove);
			} else {
				printf("Cannot move '%s': No such file or directory.\n", resource);
			}
		}

		dir_free(destinationDir);
	}
}

void makeDir(char *dirName) {
	if (!isNull(dirName)) {

		dir_t *dir = dir_create();
		dir->name = strdup(dirName);
		strcpy(dir->parentId, currentDirId);

		if (!filesystem_addDir(dir)) {
			printf("Cannot create directory '%s': Directory or file already exists with that name.\n", dirName);
		}

		dir_free(dir);
	}
}

void changeDir(char *dirName) {
	char *newDirPrompt;
	dir_t *changeToDir = filesystem_resolveDirPath(dirName ? dirName : "/", currentDirId, currentDirPrompt, &newDirPrompt);
	if (changeToDir) {
		strcpy(currentDirId, changeToDir->id);
		strcpy(currentDirPrompt, newDirPrompt);
		free(newDirPrompt);
		dir_free(changeToDir);
	} else {
		printf("cd %s: No such directory.\n", dirName);
	}
}

void listResources() {
	printf("\n");

	void printDir(dir_t *dir) {
		printf("\t\t\t" ANSI_COLOR_BLUE " %s" ANSI_COLOR_RESET "/\n", dir->name);
		//printf("\t\t\t %s/ \n", dir->name);
	}

	printf("\t\t\t" ANSI_COLOR_BLUE " %s" ANSI_COLOR_RESET "/\n", ".");
	printf("\t\t\t" ANSI_COLOR_BLUE " %s" ANSI_COLOR_RESET "/\n", "..");

	t_list *dirs = filesystem_getDirsInDir(currentDirId);
	list_iterate(dirs, (void*) printDir);
	list_destroy_and_destroy_elements(dirs, (void*) dir_free);

	void printFile(file_t *file) {
		printf("\t%-16zu %s \n", file->size, file->name);
	}

	t_list *files = filesystem_getFilesInDir(currentDirId);
	list_iterate(files, (void*) printFile);
	list_destroy_and_destroy_elements(files, (void*) file_free);
}

void copyFile(char **parameters) {
	char *option = parameters[1];
	char *source = parameters[2];
	char *dest = parameters[3];

	char* getFileName(char *path) {
		char *fileName;
		char **dirNames = string_split(path, "/");
		int i = 0;
		while (dirNames[i]) {
			fileName = dirNames[i];
			i++;
		}
		fileName = strdup(fileName);
		freeSplits(dirNames);

		return fileName;
	}

	if (!isNull(option) && !isNull(source) && !isNull(dest)) {

		if (string_equals_ignore_case(option, "-fromfs")) {
			char *localFileName = getFileName(source);

			printf("Copying file '%s' from local filesystem to '%s'\n", localFileName, dest);

			file_t *file = file_create();

			dir_t *dir = filesystem_resolveDirPath(dest, currentDirId, currentDirPrompt, NULL);
			if (dir) {
				file->name = strdup(localFileName);
				strcpy(file->parentId, dir->id);
				file->size = 0;
				dir_free(dir);
			} else {
				char *destFileName = getFileName(dest);
				if (strlen(destFileName) < strlen(dest)) {
					char *pathToFolder = string_substring_until(dest, strlen(dest) - strlen(destFileName));
					dir_t *dir = filesystem_resolveDirPath(pathToFolder, currentDirId, currentDirPrompt, NULL);
					free(pathToFolder);
					if (dir) {
						file->name = strdup(destFileName);
						strcpy(file->parentId, dir->id);
						file->size = 0;
						dir_free(dir);
					} else {
						printf("Cannot create file '%s': No such file or directory.\n", dest);
						free(destFileName);
						free(localFileName);
						file_free(file);
						return;
					}
				} else {
					// There are no folders just a file in the current folder.
					file->name = strdup(destFileName);
					strcpy(file->parentId, currentDirId);
					file->size = 0;
				}
				free(destFileName);
			}

			int result = filesystem_copyFileFromFS(source, file);
			if (result == -1) {
				printf("Cannot create file '%s': Directory or file already exists with that name.\n", dest);
			} else if (result == -2) {
				printf("Cannot create file '%s': There is no file '%s' at local filesystem.\n", dest, source);
			} else if (result == -3) {
				printf("Cannot create file '%s': There is not enough space to make all the copies.\n", dest);
			} else if (result == -4) {
				printf("Cannot create file '%s': Something went wrong when trying to persist the information.\n", dest);
			}

			free(localFileName);
			file_free(file);
		} else if (string_equals_ignore_case(option, "-tofs")) {
			file_t *file = filesystem_resolveFilePath(source, currentDirId, currentDirPrompt);
			if (file) {
				printf("Copying file '%s' to local filesystem to '%s'\n", source, dest);
				int result = filesystem_saveFileToLocalFS(file, dest);

				if (result == 1) {
					printf("Done: A file at the local filesystem named '%s' has been saved with the contents.\n", dest);
				} else if (result == -1) {
					printf("Aborting: The file '%s' is unavailable because some block couldn't be found in any active node.\n", file->name);
				}
				file_free(file);
			} else {
				printf("File '%s' not found.\n", source);
			}
		} else {
			printf("Invalid option '%s' \n", option);
		}
	}
}

void md5sum(char *filePath) {
	if (!isNull(filePath)) {

		file_t *file = filesystem_resolveFilePath(filePath, currentDirId, currentDirPrompt);
		if (file) {
			char *md5sum = filesystem_md5sum(file);
			if (md5sum) {
				printf("%s\t%s\n", md5sum, file->name);
				free(md5sum);
			} else {
				printf("Aborting: The file '%s' is unavailable because some block couldn't be found in any active node.\n", file->name);
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", filePath);
		}
	}
}

void saveBlockContents(char **parameters) {
	char *filePath = parameters[1];
	char *blockN = parameters[2];

	if (!isNull(filePath) && !isNull(blockN)) {
		int blockNumber = getIntFromString(blockN);
		if (blockNumber < 0) {
			printf("Invalid blockNumber '%s'\n", blockN);
			return;
		}

		file_t *file = filesystem_resolveFilePath(filePath, currentDirId, currentDirPrompt);
		if (file) {
			printf("Getting block %d from file '%s'\n", blockNumber, file->name);

			char tempFilePath[512];
			snprintf(tempFilePath, sizeof(tempFilePath), "/tmp/MDFS_FILE_BLOCK_%s__%d", file->name, blockNumber);
			int result = filesystem_saveFileBlockToLocalFS(file, blockNumber, tempFilePath);

			if (result == 1) {
				printf("Done: A file at the local filesystem named '%s' has been saved with the contents.\n", tempFilePath);
			} else if (result == -1) {
				printf("Aborting: The file '%s' does not have a blockNumber '%d'\n", file->name, blockNumber);
			} else if (result == -2) {
				printf("Aborting: The blockNumber '%d' is unavailable because all node copies are down.\n", blockNumber);
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", filePath);
		}
	}
}

void copyBlock(char **parameters) {
	char *filePath = parameters[1];
	char *blockN = parameters[2];

	if (!isNull(filePath) && !isNull(blockN)) {
		int blockNumber = getIntFromString(blockN);
		if (blockNumber < 0) {
			printf("Invalid blockNumber '%s'\n", blockN);
			return;
		}

		file_t *file = filesystem_resolveFilePath(filePath, currentDirId, currentDirPrompt);
		if (file) {
			printf("Making a new copy of the block %d from file '%s'\n", blockNumber, file->name);

			int result = filesystem_makeNewFileBlockCopy(file, blockNumber);

			if (result == 1) {
				printf("Done: A new copy has been made\n");
			} else if (result == -1) {
				printf("Aborting: The file '%s' does not have a blockNumber '%d'\n", file->name, blockNumber);
			} else if (result == -2) {
				printf("Aborting: The blockNumber '%d' is unavailable because all node copies are down.\n", blockNumber);
			} else if (result == -3) {
				printf("Aborting: There is no node candidate to make the copy.\n");
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", filePath);
		}
	}
}

void deleteBlock(char **parameters) {
	char *filePath = parameters[1];
	char *blockN = parameters[2];
	char *copyN = parameters[3];

	if (!isNull(filePath) && !isNull(blockN) && !isNull(copyN)) {
		int blockNumber = getIntFromString(blockN);
		if (blockNumber < 0) {
			printf("Invalid blockNumber '%s'\n", blockN);
			return;
		}
		int copyNumber = getIntFromString(copyN);
		if (copyNumber < 0) {
			printf("Invalid copyNumber '%s'\n", copyN);
			return;
		}

		file_t *file = filesystem_resolveFilePath(filePath, currentDirId, currentDirPrompt);
		if (file) {
			printf("Deleting copy %d of the block %d from file '%s'\n", copyNumber, blockNumber, file->name);

			int result = filesystem_deleteFileBlockCopy(file, blockNumber, copyNumber);

			if (result == 1) {
				printf("Done: the copy has been deleted\n");
			} else if (result == -1) {
				printf("Aborting: The file '%s' does not have a blockNumber '%d'\n", file->name, blockNumber);
			} else if (result == -2) {
				printf("Aborting: The blockBumber '%d' does not have a copyNumber '%d'\n", blockNumber, copyNumber);
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", filePath);
		}
	}
}

void printNodeStatus(char *nodeName) {
	if (!isNull(nodeName)) {
		node_t *node = filesystem_getNodeById(nodeName);
		if (node) {
			node_printBlocksStatus(node);
			node_free(node);
		} else {
			printf("Node '%s' does not exist.\n", nodeName);
		}
	}
}

void enableNode(char *nodeName) {
	if (!isNull(nodeName)) {
		node_t *node = filesystem_getNodeById(nodeName);
		if (node) {
			if (!filesystem_activateNode(node)) {
				printf("Node '%s' is not connected or is already activated!\n", nodeName);
			}
			node_free(node);
		} else {
			printf("Node '%s' does not exist.\n", nodeName);
		}
	}
}

void disableNode(char *nodeName) {
	if (!isNull(nodeName)) {
		node_t *node = filesystem_getNodeById(nodeName);
		if (node) {
			if (!filesystem_deactivateNode(node)) {
				printf("Node '%s' is not connected or is already deactivated!\n", nodeName);
			}
			node_free(node);
		} else {
			printf("Node '%s' does not exist.\n", nodeName);
		}
	}
}

void printFileBlocks(char *filePath) {
	if (!isNull(filePath)) {
		file_t *file = filesystem_resolveFilePath(filePath, currentDirId, currentDirPrompt);
		if (file) {
			int blockNumber = 0;
			printf("Blocks of file %s . It has %d blocks:\n", file->name, list_size(file->blocks));
			void listBlocks(t_list* blockCopies) {
				int copyNumber = 0;
				void listBlockCopies(file_block_t *blockCopy) {
					printf("  |  +----> Copy number %d -> Node: %s. Block number: %d\n", copyNumber, blockCopy->nodeId, blockCopy->blockIndex);
					copyNumber++;
				}
				printf("  +----> Block number %d. It has %d copies\n", blockNumber, list_size(blockCopies));
				list_iterate(blockCopies, (void *) listBlockCopies);
				blockNumber++;
			}
			list_iterate(file->blocks, (void *) listBlocks);
			printf("---------------------------------------------------------\n");

			file_free(file);
		} else {
			printf("File '%s' not found.\n", filePath);
		}
	}
}

int getIntFromString(char *string) {
	char *stringTail = string;
	errno = 0;
	unsigned long val = strtoul(string, &stringTail, 10);
	if (errno != 0 || string == stringTail || *stringTail != 0) {
		return -1;
	}
	return val;
}

void help() {
	printf("\n" ANSI_COLOR_BOLDWHITE "FILESYSTEM" ANSI_COLOR_RESET "\n");
	printf("\t format\t\t\t\t Formats MDFS\n");
	printf("\t df\t\t\t\t Prints the MDFS free space\n");

	printf("\n" ANSI_COLOR_BOLDWHITE "FILES" ANSI_COLOR_RESET "\n");
	printf("\t rm <file>\t\t\t Deletes the file <file> \n");
	printf("\t mv <file> <dest>\t\t Moves the file named <file> to the dir named <dir>\n");
	printf("\t ll\t\t\t\t Lists all the files and dirs in the current dir\n");
	printf("\t md5sum <file>\t\t\t Gets the MD5 check sum of the file named <file>\n");
	printf("\t cp -tofs <file> <dest>\t\t Copies the file <file> from the MDFS to the local FileSystem at <dest>\n");
	printf("\t cp -fromfs <file> <dest>\t Copies the file <file> from the local FileSystem to the MDFS at <dest>\n");
	printf("\t blocks <file>\t\t\t Prints the blocks status of the file <file>\n");

	printf("\n" ANSI_COLOR_BOLDWHITE "DIRECTORIES" ANSI_COLOR_RESET "\n");
	printf("\t rm -r <dir>\t\t\t Deletes the dir <dir>\n");
	printf("\t mv <dir> <dest>\t\t Moves the dir named <dir> to the dir named <dir>\n");
	printf("\t mkdir <dir>\t\t\t Makes a new dir in the current dir named <dir>\n");

	printf("\n" ANSI_COLOR_BOLDWHITE "BLOCKS" ANSI_COLOR_RESET "\n");
	printf("\t catb <file> <blockN>\t\t Gets contents of the block number <blockN> of the file <file> and saves it to a temp file\n");
	printf("\t cpb <file> <blockN>\t\t Makes a new copy (in a different node) of the block number <blockN> of the file <file>\n");
	printf("\t rmb <file> <blockN> <copyN>\t Deletes the copy <copyN> of the block number <blockN> of the file <file>\n");

	printf("\n" ANSI_COLOR_BOLDWHITE "NODES" ANSI_COLOR_RESET "\n");
	printf("\t enablen <node>\t\t\t Activates the currently connected node named <node>\n");
	printf("\t disablen <node>\t\t Deactivates the currently connected node named <node>\n");
	printf("\t nodestat <nodename>\t\t Prints the status (blocks usage) of the node named <nodename>\n");

	printf("\n" ANSI_COLOR_BOLDWHITE "OTHERS" ANSI_COLOR_RESET "\n");
	printf("\t help\t\t\t\t Prints Help (this message)\n");
	printf("\t exit\t\t\t\t Exits the MDFS\n\n");
}

