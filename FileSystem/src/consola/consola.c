#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>

void readCommand(char *command, int maximoLargo);
void freeSplits(char ** splits);
void formatMDFS();

void deleteResource(char **parameters);
void deleteFile(char *file);
void deleteDir(char *dir);

void moveResource(char *resource);

void createDir(char *dir);

void copyToMDFS(char *file);
void copyToFS(char *file);
void MD5(char *file);
void seeBlock(char *block);
void deleteBlock(char *block);
void copyBlock(char* block);
void upNode(char *node);
void deleteNode(char *node);
void help();
int isNullParameter(char *parameter);

void startConsole() {
	// lo defino como un char** porque necesito tener un "array" de los parametros
	char **parameters;
	//char *command;
	char command[100];
	int exit = false;

	do {
		printf("> ");
		readCommand(command, 100);

		parameters = string_split(command, " ");

		if (string_equals_ignore_case(parameters[0], "format")) {
			formatMDFS();
		} else if (string_equals_ignore_case(parameters[0], "rm")) {
			deleteResource(parameters);
		} else if (string_equals_ignore_case(parameters[0], "mv")) {
			moveResource(parameters[1]);
		} else if (string_equals_ignore_case(parameters[0], "mkdir")) {
			createDir(parameters[1]);
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
			exit = true;
		}

		freeSplits(parameters);
	} while (!exit);

}

void readCommand(char *lectura, int maximoLargo) {
	//Habria que buscar una funcion parecida a fgets, pero que no sea necesario un maximo, asi evitamos poner un maximo arbitrario
	fgets(lectura, maximoLargo, stdin);

	if ((strlen(lectura) > 1) && (lectura[strlen(lectura) - 1] == '\n')) {
		//agrego el /0 para que lo considere como una cadena
		lectura[strlen(lectura) - 1] = '\0';
	}

}

//asigna los splits en un auxiliar para ir liberandolos uno por uno
void freeSplits(char ** splits) {
	char **auxSplit = splits;

	while (*auxSplit != NULL) {
		free(*auxSplit);
		auxSplit++;
	}

	free(splits);
}

//La hice para evitar que mande el comando sin el parametro y lo tome como valido
int isNullParameter(char *parameter) {
	if (parameter == NULL) {
		printf("No puso el parametro\n");
		return 1;
	}

	return 0;
}

//Todas estas excepto el help son las que vamos a tener que ir desarrollando cuando hagamos el FileSystem
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

void deleteFile(char *file) {
	if (!isNullParameter(file)) {
		printf("Borra el archivo %s\n", file);
	}
}

void deleteDir(char *dir) {
	if (!isNullParameter(dir)) {
		printf("Borra el directorio %s\n", dir);
	}
}


void moveResource(char *resource) {
	if (!isNullParameter(resource)) {
		printf("Mueve el recurso %s\n", resource);
	}
}

void createDir(char *dir) {
	if (!isNullParameter(dir)) {
		printf("Crea el directorio %s\n", dir);
	}
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

