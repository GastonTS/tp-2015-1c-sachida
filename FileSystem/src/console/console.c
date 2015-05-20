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

void formatMDFS();

void deleteResource(char **parameters);
void deleteFile(char *fileName);
void deleteDir(char *dirName);

void moveResource(char *resource);

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
int isNullParameter(char *parameter);

char currentDirName[150];
char currentDirId[25];

void startConsole() {
	char **parameters;
	char *command = malloc(sizeof(char*));
	int exit = 0;

	strcpy(currentDirName, "/");
	strcpy(currentDirId, ROOT_DIR_ID);

	do {
		printf("%s > ", currentDirName);
		readCommand(command);

		// Ignore empty enter
		if (command[0] != '\0') {
			parameters = string_split(command, " ");

			if (string_equals_ignore_case(parameters[0], "format")) {
				formatMDFS();
			} else if (string_equals_ignore_case(parameters[0], "rm")) {
				deleteResource(parameters);
			} else if (string_equals_ignore_case(parameters[0], "mv")) {
				moveResource(parameters[1]);
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

}

void readCommand(char *input) {
	char buffer[4096];

	fgets(buffer, sizeof(buffer), stdin);

	strcpy(input, buffer);

	input[strlen(input) - 1] = '\0'; // Removes the \n

	// TODO: make this work !
	// string_trim(&input);
}

void freeSplits(char ** splits) {
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
}

int isNullParameter(char *parameter) {
	if (parameter == NULL) {
		printf("You are missing one parameter.. \n");
		return 1;
	}

	return 0;
}

bool isCurrentRootDir() {
	return string_equals_ignore_case(currentDirId, ROOT_DIR_ID);
}

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
	if (!isNullParameter(fileName)) {
		// TODO resolve dir.
		mongo_file_deleteFileByNameInDir(fileName, currentDirId);
	}
}

void deleteDir(char *dirName) {
	if (!isNullParameter(dirName)) {
		// TODO resolve dir.
		if (!mongo_dir_deleteDirByNameInDir(dirName, currentDirId)) {
			printf("An error occurred!\n");
		}
	}
}

void moveResource(char *resource) {
	if (!isNullParameter(resource)) {
		// TODO
		printf("Moves resource %s\n", resource);
	}
}

void makeFile(char *fileName) {
	if (!isNullParameter(fileName)) {
		file_t *file = malloc(sizeof(file_t));
		strcpy(file->name, fileName);
		strcpy(file->parentId, currentDirId);
		file->size = 0;
		mongo_file_save(file);
	}
}

void makeDir(char *dirName) {
	if (!isNullParameter(dirName)) {
		dir_t *dir = malloc(sizeof(dir_t));
		strcpy(dir->name, dirName);
		strcpy(dir->parentId, currentDirId);
		mongo_dir_save(dir);
	}
}

void changeDir(char *dirName) {
	if (!isNullParameter(dirName)) {
		int i = 0;
		char **dirNames;
		dirNames = string_split(dirName, "/");

		while (dirNames[i]) {
			dirName = dirNames[i];
			if (strcmp(dirName, "..") == 0) {
				if (!isCurrentRootDir()) {
					dir_t *currentDir = mongo_dir_getById(currentDirId);

					strcpy(currentDirName, string_substring_until(currentDirName, string_length(currentDirName) - string_length(currentDir->name) - 1));
					strcpy(currentDirId, currentDir->parentId);

					if (isCurrentRootDir()) {
						strcat(currentDirName, "/");
					}
				}
			} else {
				dir_t *dir = mongo_dir_getByNameInDir(dirName, currentDirId);

				if (dir) {
					if (!isCurrentRootDir()) {
						strcat(currentDirName, "/");
					}
					strcat(currentDirName, dir->name);
					strcpy(currentDirId, dir->id);
				} else {
					printf("Directory not found.\n");
				}
			}
			i++;
		}
	}
}

void listResources() {

	void printDir(dir_t *dir) {
		// printf("\t" ANSI_COLOR_BLUE " %s/ " ANSI_COLOR_RESET "\n", dir->name);
		printf("\t %s/ \n", dir->name);
	}

	t_list *dirs = mongo_dir_getByParentId(currentDirId);
	list_iterate(dirs, printDir);

	void printFile(file_t *file) {
		printf("\t %s \n", file->name);
	}

	t_list *files = mongo_file_getByParentId(currentDirId);
	list_iterate(files, printFile);
}

void MD5(char *file) {
	if (!isNullParameter(file)) {
		printf("Obtiene el MD5 de %s\n", file);
	}
}

void copyToMDFS(char *file) {
	if (!isNullParameter(file)) {
		printf("Copia el archivo %s al MDFS\n", file);
	}
}

void copyToFS(char *file) {
	if (!isNullParameter(file)) {
		printf("Copia el archivo %s al FileSystem\n", file);
	}
}

void seeBlock(char *block) {
	if (!isNullParameter(block)) {
		printf("Vee el Bloque nro %s\n", block);
	}
}

void deleteBlock(char *block) {
	if (!isNullParameter(block)) {
		printf("Borra el Bloque nro %s\n", block);
	}
}

void copyBlock(char *block) {
	if (!isNullParameter(block)) {
		printf("Copia el Bloque nro %s\n", block);
	}
}

void upNode(char *node) {
	if (!isNullParameter(node)) {
		printf("Agrega el nodo %s\n", node);
	}
}

void deleteNode(char *node) {
	if (!isNullParameter(node)) {
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

