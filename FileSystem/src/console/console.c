#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>

#include "console.h"
#include "../filesystem/filesystem.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void readCommand(char *command);

bool isCurrentRootDir();
bool isRootDir(char *dirId);
int isNull(char *parameter);

void format();
void diskFree();

void deleteResource(char **parameters);
void moveResource(char *resource, char *destination);

void copyFile(char **parameters);
void makeDir(char *dirName);
void changeDir(char *dirName);
void listResources();

void printNodeStatus(char *nodeName);
void md5sum(char *fileName);

void seeBlock(char *block);
void deleteBlock(char *block);
void copyBlock(char* block);
void upNode(char *node);
void deleteNode(char *nodeName);
void help();

char *currentDirPrompt;
char *currentDirId;

void console_start() {
	char **parameters;
	char *command = malloc(sizeof(char) * 512);
	int exit = 0;

	currentDirPrompt = malloc(sizeof(char) * 512); // TODO.. change this.
	currentDirId = malloc(ID_SIZE);

	strcpy(currentDirPrompt, "/");
	strcpy(currentDirId, ROOT_DIR_ID);

	do {
		printf("%s > ", currentDirPrompt);
		readCommand(command);
		// TODO NO ANDA EL TRIM DE COMONS .string_trim(&command);

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
			} else if (string_equals_ignore_case(parameters[0], "nodestat")) {
				printNodeStatus(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "seeBlock")) {
				seeBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "deleteBlock")) {
				deleteBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "copyBlock")) {
				copyBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "upNode")) {
				upNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "rmn")) {
				deleteNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "help")) {
				help();
			} else if (string_equals_ignore_case(parameters[0], "exit")) {
				printf("bye\n");
				exit = 1;
			} else if (string_equals_ignore_case(parameters[0], "\n")) {
				// ignore enter
			} else {
				printf("Invalid command \n");
			}
			freeSplits(parameters);
		}
	} while (!exit);

	free(command);
	free(currentDirId);
	free(currentDirPrompt);
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

bool isCurrentRootDir() {
	return isRootDir(currentDirId);
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
			if (!isNull(parameters[2])) {
				if (!filesystem_deleteDirByNameInDir(parameters[2], currentDirId)) {
					printf("Cannot remove '%s': No such directory.\n", parameters[2]);
				}
			}
		} else {
			if (!filesystem_deleteFileByNameInDir(parameters[1], currentDirId)) {
				printf("Cannot remove '%s': No such file.\n", parameters[1]);
			}
		}
	}
}

void moveResource(char *resource, char *destination) {
	if (!isNull(resource) && !isNull(destination)) {

		dir_t *destinationDir = filesystem_resolveDirPath(destination, currentDirId, currentDirPrompt, NULL);
		if (!destinationDir) {
			printf("Cannot move. Dir %s not found", destination);
			return;
		}

		dir_t *dirToMove = filesystem_getDirByNameInDir(resource, currentDirId);
		if (dirToMove) {
			filesystem_moveDir(dirToMove, destinationDir->id);
			dir_free(dirToMove);
		} else {
			// If couldn't find a dir, then try to find a file:

			file_t *fileToMove = filesystem_getFileByNameInDir(resource, currentDirId);
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
	if (!isNull(dirName)) {

		char *newDirPrompt = malloc(sizeof(char) * 512);

		dir_t *movedToDir = filesystem_resolveDirPath(dirName, currentDirId, currentDirPrompt, newDirPrompt);
		if (movedToDir) {
			strcpy(currentDirId, movedToDir->id);
			strcpy(currentDirPrompt, newDirPrompt);
			dir_free(movedToDir);
		}

		free(newDirPrompt);
	}
}

void listResources() {
	printf("\n");

	void printDir(dir_t *dir) {
		printf("\t\t\t" ANSI_COLOR_BLUE " %s/ " ANSI_COLOR_RESET "\n", dir->name);
		//printf("\t\t\t %s/ \n", dir->name);
	}

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

	if (!isNull(option) && !isNull(source) && !isNull(dest)) {

		if (string_equals_ignore_case(option, "-fromfs")) {
			char *fileName;
			char **dirNames = string_split(source, "/");
			int i = 0;
			while (dirNames[i]) {
				fileName = dirNames[i];
				i++;
			}
			printf("Copying file '%s' to MDFS as '%s'\n", fileName, dest);

			file_t *file = file_create();
			file->name = strdup(dest);
			strcpy(file->parentId, currentDirId);
			file->size = 0;

			int result = filesystem_copyFileFromFS(source, file);
			if (result == -1) {
				printf("Cannot create file '%s': Directory or file already exists with that name.\n", dest);
			} else if (result == -2) {
				printf("Cannot create file '%s': There is not enough space to make all the copies.\n", dest);
			} else if (result == -3) {
				printf("Cannot create file '%s': Something went wrong when trying to persist the information.\n", dest);
			}

			file_free(file);
			freeSplits(dirNames);
		} else if (string_equals_ignore_case(option, "-tofs")) {
			// TODO
			printf("Copia el archivo '%s' al FS: %s\n", source, dest);
		} else {
			printf("Invalid option '%s' \n", option);
		}
	}
}

void md5sum(char *fileName) {
	if (!isNull(fileName)) {
		file_t *file = filesystem_getFileByNameInDir(fileName, currentDirId);
		if (file) {
			char *md5sum = filesystem_md5sum(file);
			if (md5sum) {
				printf("%s\t%s\n", md5sum, fileName);
				free(md5sum);
			} else {
				printf("There was an error trying to get the MD5 of %s\n", fileName);
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", fileName);
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

void seeBlock(char *block) {
	if (!isNull(block)) {
		// TODO
		printf("Vee el Bloque nro '%s'\n", block);
	}
}

void deleteBlock(char *block) {
	if (!isNull(block)) {
		printf("Borra el Bloque nro '%s'\n", block);
	}
}

void copyBlock(char *block) {
	if (!isNull(block)) {
		// TODO
		printf("Copia el Bloque nro '%s'\n", block);
	}
}

void upNode(char *node) {
	if (!isNull(node)) {
		// TODO
		printf("Agrega el nodo '%s'\n", node);
	}
}

void deleteNode(char *nodeName) {
	if (!isNull(nodeName)) {
		// TODO
		printf("Borra el nodo '%s'\n", nodeName);
		filesystem_nodeIsDown(nodeName);
	}
}

void help() {
	printf("Valid commands:\n\n");
	printf("\t format\t\t\t\t Formats MDFS\n");
	printf("\t df\t\t\t\t Prints the MDFS free space\n");
	printf("\t rm <file>\t\t\t Deletes the file <file> \n");
	printf("\t rm -r <dir>\t\t\t Deletes the dir <dir>\n");
	printf("\t mv <file> <dest>\t\t Moves the file named <file> to the dir named <dir>\n");
	printf("\t mv <dir> <dest>\t\t Moves the dir named <dir> to the dir named <dir>\n");
	printf("\t mkdir <dir>\t\t\t Makes a new dir in the current dir named <dir>\n");
	printf("\t ll\t\t\t\t Lists all the files and dirs in the current dir\n");
	printf("\t md5sum <file>\t\t\t Gets the MD5 check sum of the file named <file>\n");
	printf("\t cp -tofs <file> <dest>\t\t Copies the file <file> from the MDFS to the local FileSystem at <dest>\n");
	printf("\t cp -fromfs <file> <dest>\t Copies the file <file> from the local FileSystem to the MDFS at <dest>\n");

	printf("\t nodestat <nodename>\t\t Prints the status (blocks usage) of the node named <nodename>\n");

	printf("\t help\t\t\t\t Prints Help (this message)\n");
	printf("\t exit\t\t\t\t Exits the MDFS\n\n");

	printf("\n\n CHECK:\n");
	printf("catb block		Muestra el bloque block\n");
	printf("rmb <block>		Borra el bloque block\n");
	printf("cpb <block>		Copia el bloque block\n");
	printf("mkn <node>		Agrega el nodo node\n");
	printf("rmn <node>		Borra el nodo node\n");
}

