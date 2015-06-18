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

void printNodeStatus(char *nodeName);
void md5sum(char *fileName);

void saveBlockContents(char **parameters);
void deleteBlock(char **parameters);
void copyBlock(char **parameters);

void upNode(char *node);
void deleteNode(char *nodeName);
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
			} else if (string_equals_ignore_case(parameters[0], "catb")) {
				saveBlockContents(parameters);
			} else if (string_equals_ignore_case(parameters[0], "cpb")) {
				copyBlock(parameters);
			} else if (string_equals_ignore_case(parameters[0], "rmb")) {
				deleteBlock(parameters);
			} else if (string_equals_ignore_case(parameters[0], "upNode")) {
				upNode(parameters[1]);
			} else if (string_equals_ignore_case(parameters[0], "rmn")) {
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
			printf("Cannot move. Dir %s not found", destination);
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
				printf("Cannot create file '%s': There is no file %s at local filesystem.\n", dest, source);
			} else if (result == -3) {
				printf("Cannot create file '%s': There is not enough space to make all the copies.\n", dest);
			} else if (result == -4) {
				printf("Cannot create file '%s': Something went wrong when trying to persist the information.\n", dest);
			}

			free(localFileName);
			file_free(file);
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

		file_t *file = filesystem_resolveFilePath(fileName, currentDirId, currentDirPrompt);
		if (file) {
			char *md5sum = filesystem_md5sum(file);
			if (md5sum) {
				printf("%s\t%s\n", md5sum, file->name);
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

void saveBlockContents(char **parameters) {
	char *fileName = parameters[1];
	char *blockN = parameters[2];

	if (!isNull(fileName) && !isNull(blockN)) {
		int blockNumber = getIntFromString(blockN);
		if (blockNumber < 0) {
			printf("Invalid blockNumber '%s'\n", blockN);
			return;
		}

		file_t *file = filesystem_resolveFilePath(fileName, currentDirId, currentDirPrompt);
		if (file) {
			printf("Getting block %d from file '%s'\n", blockNumber, file->name);

			char tempFileName[512];
			snprintf(tempFileName, sizeof(tempFileName), "/tmp/MDFS_FILE_BLOCK_%s__%d", file->name, blockNumber);
			int result = filesystem_saveFileBlockToFile(file, blockNumber, tempFileName);

			// TODO mostrar las copias (nombre, index)
			if (result == 1) {
				printf("Done: A file at the local filesystem named '%s' has been saved with the contents.\n", tempFileName);
			} else if (result == -1) {
				printf("Aborting: The file '%s' does not have a blockNumber '%d'\n", file->name, blockNumber);
			} else if (result == -2) {
				printf("Aborting: The blockNumber '%d' is unavailable because all node copies are down.\n", blockNumber);
			}
			file_free(file);
		} else {
			printf("File '%s' not found.\n", fileName);
		}
	}
}

void copyBlock(char **parameters) {
	char *fileName = parameters[1];
	char *blockN = parameters[2];

	if (!isNull(fileName) && !isNull(blockN)) {
		int blockNumber = getIntFromString(blockN);
		if (blockNumber < 0) {
			printf("Invalid blockNumber '%s'\n", blockN);
			return;
		}

		file_t *file = filesystem_resolveFilePath(fileName, currentDirId, currentDirPrompt);
		if (file) {
			printf("Making a new copy of the block %d from file '%s'\n", blockNumber, file->name);

			int result = filesystem_makeNewFileBlockCopy(file, blockNumber);

			// TODO mostrar la nueva copia (nombre, index)
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
			printf("File '%s' not found.\n", fileName);
		}
	}
}

void deleteBlock(char **parameters) {
	if (!isNull(parameters[1])) {
		printf("Borra el Bloque nro '%s'\n", parameters[1]);
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

int getIntFromString(char *string) {
	char *stringTail = string;
	errno = 0;
	unsigned long val = strtoul(string, &stringTail, 10);
	if (errno != 0 || string == stringTail || *stringTail != 0) {
		return -1;
	}
	return val;
}

/*
 *
 * Bueno, tiro la consulta entonces, no es compleja, aunque parezca largo todo lo que escriba. La consulta es de enunciado ya que no tuve tiempo de preguntar el sabado pasado. Las operaciones sobre bloques desde la consola del FS, como son especificamente? Les comento lo que yo entiendo y diganme si esta ok o corrijanme.
 1- Mostrar bloque: Se pasa un archivo y un nro de bloque y se muestra las copias de ese bloque por pantalla, indicando el nodo y nro bloque dentro de ese nodo
 2- Copiar bloque: Se le pasa un archivo y un nro de bloque y se genera una nueva copia en un nodo (siempre respetando el balance de los mismos y que no se repita la misma copia en el mismo nodo)
 3- Borrar bloque: Este es el que mas dudas me genera, que se hace? Se borran TODAS las copias de un bloque dado? Si es asi, el archivo quedaria como no disponible.
 1.- y el contenido
 queremos ver lo que hay dentro
 2.- en este punto hay un debate enérgico interno
 en principio es exactamente como decís vos
 lo que se está debatiendo es el tema de si un archivo soporta más de tres copias de bloque
 en principio venía por el lado de si tenés un nodo caído, copiás un bloque a otro lado de una copia que esté funcionando
 3.- borrarías una copia. es el análogo a copiar bloque
 el opuesto quise decir
 Habian dicho que en vez de20mb por pantalla los mandes a un archivo

 */

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

	printf("\t catb <file> <blockN>\t\t Gets contents of the block number <blockN> (zero-based) of the file <file> and saves it to a temp file\n");
	printf("\t cpb <file> <blockN>\t\t Makes a new copy (in a different node) of the block number <blockN> (zero-based) of the file <file>\n");
	printf("\t rmb <file> <blockN> <copyN>\t Deletes the copy <copyN> (zero-based) of the block number <blockN> (zero-based) of the file <file>\n");

	printf("\t help\t\t\t\t Prints Help (this message)\n");
	printf("\t exit\t\t\t\t Exits the MDFS\n\n");

	printf("\n\n CHECK:\n");

	printf("mkn <node>		Agrega el nodo node\n");
	printf("rmn <node>		Borra el nodo node\n");
}

