#include <stdio.h>
#include <stdlib.h>

#include "mongo/mongo.h"
#include "console/console.h"

void testCreateFile();

int main(void) {
	mongo_init();
	testCreateFile();
	startConsole();
	return EXIT_SUCCESS;
}

void testCreateFile() {
	file_t *file = malloc(sizeof(file_t));
	strcpy(file->name, "Nombre del archivo de test");
	file->size = 1234;
	strcpy(file->parentId, "5554545454TESTTEST");

	mongo_saveFile(file);

	printf("ID generated: %s\n", file->id);

	mongo_getFile(file->id);
}
