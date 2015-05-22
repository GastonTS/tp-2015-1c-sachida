#include <stdio.h>
#include <stdlib.h>
#include <commons/string.h>
#include <string.h>

#include "console.h"
#include "../mongo/mongo_dir.h"
#include "../mongo/mongo_file.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void readCommand(char *command);
void freeSplits(char ** splits);

bool isCurrentRootDir();
bool isRootDir(char *dirId);
bool resolveDir(char *dirPath, char *dirPrompt, char *dirId);

void format();

void deleteResource(char **parameters);
void deleteFile(char *fileName);
void deleteDir(char *dirName);

void moveResource(char *resource, char *destination);

void makeFile(char *fileName);
void makeDir(char *dirName);
void changeDir(char *dirName);

void listResources();

void copyFile(char **parameters);
void md5sum(char *fileName);
void seeBlock(char *block);
void deleteBlock(char *block);
void copyBlock(char* block);
void upNode(char *node);
void deleteNode(char *node);
void help();
int isNull(char *parameter);

char *currentDirPrompt;
char *currentDirId;

void startConsole() {
	char **parameters;
	char *command = malloc(sizeof(char) * 512);
	int exit = 0;

	currentDirPrompt = malloc(sizeof(char) * 512);
	currentDirId = malloc(sizeof(char) * 25);

	strcpy(currentDirPrompt, "/");
	strcpy(currentDirId, ROOT_DIR_ID);

	do {
		printf("%s > ", currentDirPrompt);
		readCommand(command);

		// Ignore empty enter
		if (command[0] != '\0') {
			parameters = string_split(command, " ");

			if (string_equals_ignore_case(parameters[0], "format")) {
				format();
			} else if (string_equals_ignore_case(parameters[0], "rm")) {
				deleteResource(parameters);
			} else if (string_equals_ignore_case(parameters[0], "mv")) {
				moveResource(parameters[1], parameters[2]);
			} else if (string_equals_ignore_case(parameters[0], "mkdir")) {
				makeDir(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "touch")) {
				makeFile(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "cd")) {
				changeDir(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "ll")) {
				listResources();
			} else if (string_equals_ignore_case(parameters[0], "md5sum")) {
				md5sum(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "cp")) {
				copyFile(parameters);
			} else if (string_equals_ignore_case(parameters[0], "seeBlock")) {
				seeBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "deleteBlock")) {
				deleteBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "copyBlock")) {
				copyBlock(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "upNode")) {
				upNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "deleteNode")) {
				deleteNode(parameters[1]);
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

	free(command);
	free(currentDirId);
	free(currentDirPrompt);
}

void readCommand(char *input) {
	char buffer[4096];

	fgets(buffer, sizeof(buffer), stdin);

	strcpy(input, buffer);

	input[strlen(input) - 1] = '\0'; // Removes the \n

	// TODO: make this work !
	// string_trim(&input);
}

void freeSplits(char **splits) {
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
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

bool isRootDir(char *dirId) {
	return string_equals_ignore_case(dirId, ROOT_DIR_ID);
}

// Resolves a path like: folder/../folder/folder2/../..
bool resolveDir(char *dirPath, char *dirPrompt, char *dirId) {
	char **dirNames;
	char *dirName;
	int i = 0;

	char *newDirPrompt = malloc(sizeof(char) * 512);
	char *newDirId = malloc(sizeof(char) * 25);

	strcpy(newDirPrompt, currentDirPrompt);
	strcpy(newDirId, currentDirId);

	dirNames = string_split(dirPath, "/");

	while (dirNames[i]) {
		dirName = dirNames[i];

		if (strcmp(dirName, "") != 0) {
			if (strcmp(dirName, "..") == 0) {
				if (!isRootDir(newDirId)) {
					dir_t *currentDir = mongo_dir_getById(newDirId);

					// Removes the last folder in the prompt
					newDirPrompt[string_length(newDirPrompt) - string_length(currentDir->name) - 1] = '\0';

					strcpy(newDirId, currentDir->parentId);

					if (isRootDir(newDirId)) {
						strcat(newDirPrompt, "/");
					}
					dir_free(currentDir);
				}
			} else {
				dir_t *dir = mongo_dir_getByNameInDir(dirName, newDirId);

				if (dir) {
					if (!isRootDir(newDirId)) {
						strcat(newDirPrompt, "/");
					}
					strcat(newDirPrompt, dir->name);
					strcpy(newDirId, dir->id);

					dir_free(dir);
				} else {
					printf("Directory %s not found.\n", dirName);
					return 0;
				}
			}
		}
		i++;
	}

	if (dirPrompt) {
		strcpy(dirPrompt, newDirPrompt);
	}
	if (dirId) {
		strcpy(dirId, newDirId);
	}

	free(newDirPrompt);
	free(newDirId);
	freeSplits(dirNames);

	return 1;
}

// FUNCTIONS ..

void format() {
	// TODO
	printf("Formatea el MDFS\n");
}

void deleteResource(char **parameters) {
	if (string_equals_ignore_case(parameters[1], "-r")) {
		deleteDir(parameters[2]);
	} else {
		deleteFile(parameters[1]);
	}
}

void deleteFile(char *fileName) {
	if (!isNull(fileName)) {
		mongo_file_deleteFileByNameInDir(fileName, currentDirId);
	}
}

void deleteDir(char *dirName) {
	if (!isNull(dirName)) {
		mongo_dir_deleteDirByNameInDir(dirName, currentDirId);
	}
}

void moveResource(char *resource, char *destination) {
	if (!isNull(resource) && !isNull(destination)) {

		char *destinationId = malloc(sizeof(char) * 25);

		dir_t *dirToMove = mongo_dir_getByNameInDir(resource, currentDirId);
		if (dirToMove) {
			if (resolveDir(destination, NULL, destinationId)) {
				mongo_dir_updateParentId(dirToMove->id, destinationId);
			}
			dir_free(dirToMove);
		} else {
			// If couldn't find a dir, then try to find a file:

			file_t *fileToMove = mongo_file_getByNameInDir(resource, currentDirId);
			if (fileToMove) {
				if (resolveDir(destination, NULL, destinationId)) {
					mongo_file_updateParentId(fileToMove->id, destinationId);
				}
				file_free(fileToMove);
			} else {
				printf("Directory or file %s not found.\n", resource);
			}
		}

		free(destinationId);
	}
}

void makeFile(char *fileName) {
	if (!isNull(fileName)) {
		if (mongo_dir_getByNameInDir(fileName, currentDirId)) {
			printf("Cannot create file %s: Directory exists\n", fileName);
			return;
		}

		file_t *file = file_create();

		strcpy(file->name, fileName);
		strcpy(file->parentId, currentDirId);
		file->size = 0;
		mongo_file_save(file);

		file_free(file);
	}
}

void makeDir(char *dirName) {
	if (!isNull(dirName)) {
		if (mongo_file_getByNameInDir(dirName, currentDirId)) {
			printf("Cannot create directory %s: File exists\n", dirName);
			return;
		}

		dir_t *dir = dir_create();

		strcpy(dir->name, dirName);
		strcpy(dir->parentId, currentDirId);
		mongo_dir_save(dir);

		dir_free(dir);
	}
}

void changeDir(char *dirName) {
	if (!isNull(dirName)) {

		char *newDirPrompt = malloc(sizeof(char) * 512);
		char *newDirId = malloc(sizeof(char) * 25);

		if (resolveDir(dirName, newDirPrompt, newDirId)) {
			// TODO, igualar al puntero haciendo previous free?
			strcpy(currentDirId, newDirId);
			strcpy(currentDirPrompt, newDirPrompt);
		}

		free(newDirPrompt);
		free(newDirId);
	}
}

void listResources() {

	void printDir(dir_t *dir) {
		// printf("\t" ANSI_COLOR_BLUE " %s/ " ANSI_COLOR_RESET "\n", dir->name);
		printf("\t %s/ \n", dir->name);
	}

	t_list *dirs = mongo_dir_getByParentId(currentDirId);
	list_iterate(dirs, (void*) printDir);
	list_destroy_and_destroy_elements(dirs, (void*) dir_free);

	void printFile(file_t *file) {
		printf("\t %s \n", file->name);
	}

	t_list *files = mongo_file_getByParentId(currentDirId);
	list_iterate(files, (void*) printFile);
	list_destroy_and_destroy_elements(files, (void*) file_free);
}

void md5sum(char *file) {
	if (!isNull(file)) {
		// TODO
		printf("Obtiene el MD5 de %s\n", file);
	}
}

void copyFile(char **parameters) {
	char *option = parameters[1];
	char *source = parameters[2];
	char *dest = parameters[3];

	if (!isNull(option) && !isNull(source) && !isNull(dest)) {
		// TODO
		if (string_equals_ignore_case(option, "-fromfs")) {
			char *file;
			char **dirNames = string_split(source, "/");
			int i = 0;
			while(dirNames[i]) {
				file = dirNames[i];
				i++;
			}
			printf("Copia el archivo %s al MDFS: %s\n", file, dest);
			freeSplits(dirNames);
		} else if (string_equals_ignore_case(option, "-tofs")) {
			printf("Copia el archivo %s al FS: %s\n", source, dest);
		} else {
			printf ("Invalid option %s \n", option);
		}
	}
}

void seeBlock(char *block) {
	if (!isNull(block)) {
		// TODO
		printf("Vee el Bloque nro %s\n", block);
	}
}

void deleteBlock(char *block) {
	if (!isNull(block)) {
		printf("Borra el Bloque nro %s\n", block);
	}
}

void copyBlock(char *block) {
	if (!isNull(block)) {
		// TODO
		printf("Copia el Bloque nro %s\n", block);
	}
}

void upNode(char *node) {
	if (!isNull(node)) {
		// TODO
		printf("Agrega el nodo %s\n", node);
	}
}

void deleteNode(char *node) {
	if (!isNull(node)) {
		// TODO
		printf("Borra el nodo %s\n", node);
	}
}

void help() {
	printf("Valid commands:\n\n");
	printf("\t format\t\t\t\t Formats MDFS\n");
	printf("\t rm <file>\t\t\t Deletes the file <file> \n");
	printf("\t rm -r <dir>\t\t\t Deletes the dir <dir>\n");
	printf("\t mv <file> <dest>\t\t Moves the file named <file> to the dir named <dir>\n");
	printf("\t mv <dir> <dest>\t\t Moves the dir named <dir> to the dir named <dir>\n");
	printf("\t mkdir <dir>\t\t\t Makes a new dir in the current dir named <dir>\n");
	printf("\t ll\t\t\t\t Lists all the files and dirs in the current dir\n");
	printf("\t md5sum <file>\t\t\t Gets the MD5 check sum of the file named <file>\n");
	printf("\t cp -tofs <file> <dest>\t\t Copies the file <file> from the MDFS to the local FileSystem at <dest>\n");
	printf("\t cp -fromfs <file> <dest>\t Copies the file <file> from the local FileSystem to the MDFS at <dest>\n");
	printf("\t help\t\t\t\t Prints Help (this message)\n");
	printf("\t exit\t\t\t\t Exits the MDFS\n\n");

	printf("\n\n CHECK:\n");
	printf("seeBlock block		Muestra el bloque block\n");
	printf("deleteBlock block	Borra el bloque block\n");
	printf("copyBlock block		Copia el bloque block\n");
	printf("upNode node		Agrega el nodo node\n");
	printf("deleteNode node		Borra el nodo node\n");
}

