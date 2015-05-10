#include <stdio.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include <commons/string.h>
#include <string.h>

void readCommand(char *command, int maximoLargo);
void freeSplits(char ** splits);
void formatMDFS();
void deleteFile(char *file);
void renameFile(char *file);
void moveFile(char *file);
void createDir(char *dir);
void deleteDir(char *dir);
void renameDir(char *dir);
void moveDir(char *dir);
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

//Incializo los comandos en un array para despues poder hacer
//el switch y llamar a la funcion correspondiente
const char* const validCommands[]= {
				"format",
				"deleteFile",
				"renameFile",
				"moveFile",
				"createDir",
				"deleteDir",
				"renameDir",
				"moveDir",
				"copyToMDFS",
				"copyToFS",
				"MD5",
				"seeBlock",
				"deleteBlock",
				"copyBlock",
				"upNode",
				"deleteNode",
				"help",
				"exitMDFS"	//Aunque no tenga una funcion, lo asigno asi no aparece comando invalido cuando salgo
};

void main() {
	// // lo defino como un char** porque necesito tener un "array"
	//con todas las cadenas, donde la primera es el comando y las demas los parametros
	char **splitCommand;
	char command[100];
	int exit;
	int i;

	do {
		printf(">");
		int option = 18; //18 es la opcion de comando Invalido
		readCommand(command, 100);
		printf("\n");

		//Cuando recibo el comando lo separo en comando-parametros (1 por ahora)
		splitCommand = string_split(command, " ");

		//Una vez que lo separe reviso si la primera parte de los splits splitCommand[0]
		//se corresponde con alguno de los comandos validos para asignar la opcion
		for (i = 0; i < 18; i++) {
			if (string_equals_ignore_case(splitCommand[0], validCommands[i])) {
				option = i;
			}
		}

		switch (option) {
		//Uso splitCommand[1] porque es la parte de los parametros cuando agreguemos mas parametros
		//a las funciones deberiamos revisar cuantos parametros le mandamos y mandarlos asi:
		//por ejemplo para 3 (splitCommand[1],splitCommand[3],splitCommand[3])
		case 0:
			formatMDFS();
			break;
		case 1:
			deleteFile(splitCommand[1]);
			break;
		case 2:
			renameFile(splitCommand[1]);
			break;
		case 3:
			moveFile(splitCommand[1]);
			break;
		case 4:
			createDir(splitCommand[1]);
			break;
		case 5:
			deleteDir(splitCommand[1]);
			break;
		case 6:
			renameDir(splitCommand[1]);
			break;
		case 7:
			moveDir(splitCommand[1]);
			break;
		case 8:
			copyToMDFS(splitCommand[1]);
			break;
		case 9:
			copyToFS(splitCommand[1]);
			break;
		case 10:
			MD5(splitCommand[1]);
			break;
		case 11:
			seeBlock(splitCommand[1]);
			break;
		case 12:
			deleteBlock(splitCommand[1]);
			break;
		case 13:
			copyBlock(splitCommand[1]);
			break;
		case 14:
			upNode(splitCommand[1]);
			break;
		case 15:
			deleteNode(splitCommand[1]);
			break;
		case 16:
			help();
			break;
		case 18:
			printf("Comando Invalido\n");
			break;

		}
	exit = string_equals_ignore_case(splitCommand[0], "exitMDFS");

	//string_n_splits supongo que va haciendo malloc o algo parecido para ir asignando las cadenas,
	//para evitar que halla perdidas de memoria, esta funcion va liberando uno por uno los splits en splitcommand
	freeSplits(splitCommand);
	} while (!exit);

}

void readCommand(char *lectura, int maximoLargo) {
	//Habria que buscar una funcion parecida a fgets, pero que no sea necesario un maximo, asi evitamos poner un maximo arbitrario
	fgets(lectura, maximoLargo, stdin);

	if ((strlen(lectura) > 0) && (lectura[strlen(lectura) - 1] == '\n')) {
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

void deleteFile(char *file) {
	if (!isNullParameter(file))
		printf("Borra el archivo %s\n", file);
}

void renameFile(char *file) {
	if (!isNullParameter(file))
		printf("Renombra el archivo %s\n", file);
}

void moveFile(char *file) {
	if (!isNullParameter(file))
		printf("Mueve el archivo %s\n", file);
}

void createDir(char *dir) {
	if (!isNullParameter(dir))
		printf("Crea el directorio %s\n", dir);
}

void deleteDir(char *dir) {
	if (!isNullParameter(dir))
		printf("Borra el directorio %s\n", dir);
}

void renameDir(char *dir) {
	if (!isNullParameter(dir))
		printf("Renombra el directorio %s\n", dir);
}

void moveDir(char *dir) {
	if (!isNullParameter(dir))
		printf("Mueve el directorio %s\n", dir);
}

void copyToMDFS(char *file) {
	if (!isNullParameter(file))
		printf("Copia el archivo %s al MDFS\n", file);
}

void copyToFS(char *file) {
	if (!isNullParameter(file))
		printf("Copia el archivo %s al FileSystem\n", file);
}

void MD5(char *file) {
	if (!isNullParameter(file))
		printf("Obtiene el MD5 de %s\n", file);
}

void seeBlock(char *block) {
	if (!isNullParameter(block))
		printf("Vee el Bloque nro %s\n", block);
}

void deleteBlock(char *block) {
	if (!isNullParameter(block))
		printf("Borra el Bloque nro %s\n", block);
}

void copyBlock(char *block) {
	if (!isNullParameter(block))
		printf("Copia el Bloque nro %s\n", block);
}

void upNode(char *node) {
	if (!isNullParameter(node))
		printf("Agrega el nodo %s\n", node);
}

void deleteNode(char *node) {
	if (!isNullParameter(node))
		printf("Borra el nodo %s\n", node);
}

void help() {
	printf("Comandos Validos\n");
	printf("formatMDFS		Formatea el MDFS\n");
	printf("deleteFile file		Borra el archivo file\n");
	printf("renameFile file		Renombra el archivo file\n");
	printf("moveFile file		Mueve el archivo file\n");
	printf("createDir dir		Crea un directorio llamado dir\n");
	printf("deleteDir dir		Borra el directorio dir\n");
	printf("renameDir dir		Renombra el directorio dir\n");
	printf("moveDir dir		Mueve el directorio dir\n");
	printf("copyToMDFS file		Copia el archivo file al MDFS\n");
	printf("copyToFS file		Copia el archivo file al File System\n");
	printf("MD5 file		Obtiene el MD5 de file\n");
	printf("seeBlock block		Muestra el bloque block\n");
	printf("deleteBlock block	Borra el bloque block\n");
	printf("copyBlock block		Copia el bloque block\n");
	printf("upNode node		Agrega el nodo node\n");
	printf("deleteNode node		Borra el nodo node\n");
	printf("exitMDFS		Cierra la consola del MDFS\n\n");
}

