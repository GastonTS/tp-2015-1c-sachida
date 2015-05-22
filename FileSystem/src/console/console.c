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
bool resolveDir(char *dirPath, char *dirPromt, char *dirId);

void formatMDFS();

void deleteResource(char **parameters);
void deleteFile(char *fileName);
void deleteDir(char *dirName);

void moveResource(char *resource, char *destination);

void makeFile(char *fileName);
void makeDir(char *dirName);
void changeDir(char *dirName);

void listResources();

void copyToMDFS(char *fileName);
void copyToFS(char *fileName);
void MD5(char *fileName);
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
				formatMDFS();
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
			} else if (string_equals_ignore_case(parameters[0], "md5")) {
				MD5(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "copyToMDFS")) {
				copyToMDFS(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "copyToFS")) {
				copyToFS(parameters[1]);
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
bool resolveDir(char *dirPath, char *dirPromt, char *dirId) {
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

	strcpy(dirPromt, newDirPrompt);
	strcpy(dirId, newDirId);

	free(newDirPrompt);
	free(newDirId);
	freeSplits(dirNames);

	return 1;
}

// FUNCTIONS ..

void formatMDFS() {
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
	if (!isNull(resource) && !isNull(resource)) {

		// TODO support for  files. (duplicate names maybe?)

		dir_t *dirToMove = mongo_dir_getByNameInDir(resource, currentDirId);

		if (dirToMove) {
			char *destinationId = malloc(sizeof(char) * 25);
			if (resolveDir(destination, NULL, destinationId)) {
				printf("Moves resource %s to %s\n", resource, destination);
			}

			free(destinationId);
			dir_free(dirToMove);
		} else {
			printf("Directory %s not found.\n", resource);
		}
	}
}

void makeFile(char *fileName) {
	if (!isNull(fileName)) {
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
	list_iterate(dirs, printDir);
	list_destroy_and_destroy_elements(dirs, dir_free);

	void printFile(file_t *file) {
		printf("\t %s \n", file->name);
	}

	t_list *files = mongo_file_getByParentId(currentDirId);
	list_iterate(files, printFile);
	list_destroy_and_destroy_elements(files, file_free);
}

void MD5(char *file) {
	if (!isNull(file)) {
		printf("Obtiene el MD5 de %s\n", file);
	}
}

void copyToMDFS(char *file) {
	if (!isNull(file)) {
		printf("Copia el archivo %s al MDFS\n", file);
	}
}

void copyToFS(char *file) {
	if (!isNull(file)) {
		printf("Copia el archivo %s al FileSystem\n", file);
	}
}

void seeBlock(char *block) {
	if (!isNull(block)) {
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
		printf("Copia el Bloque nro %s\n", block);
	}
}

void upNode(char *node) {
	if (!isNull(node)) {
		printf("Agrega el nodo %s\n", node);
	}
}

void deleteNode(char *node) {
	if (!isNull(node)) {
		printf("Borra el nodo %s\n", node);
	}
}

void help() {
	printf("Comandos Validos\n");
	printf("formatMDFS		Formatea el MDFS\n");
	printf("rm file		Borra el archivo file\n");
	printf("rm -r dir		Borra el directorio dir\n");
	printf("mv file		Mueve el archivo file\n");
	printf("mv dir		Mueve el directorio dir\n");
	printf("mkdir dir		Crea un directorio llamado dir\n");
	printf("MD5 file		Obtiene el MD5 de file\n");
	printf("copyToMDFS file		Copia el archivo file al MDFS\n");
	printf("copyToFS file		Copia el archivo file al File System\n");
	printf("seeBlock block		Muestra el bloque block\n");
	printf("deleteBlock block	Borra el bloque block\n");
	printf("copyBlock block		Copia el bloque block\n");
	printf("upNode node		Agrega el nodo node\n");
	printf("deleteNode node		Borra el nodo node\n");
	printf("exitMDFS		Cierra la consola del MDFS\n\n");
}

