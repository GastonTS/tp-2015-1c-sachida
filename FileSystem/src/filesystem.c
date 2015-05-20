#include <stdio.h>
#include <stdlib.h>

#include "mongo/mongo_file.h"
#include "mongo/mongo_dir.h"
#include "console/console.h"

void testCreateFile();

int main(void) {
	//testCreateDir();
	//testCreateFile();
	startConsole();
	return EXIT_SUCCESS;
}

void testCreateFile() {
	file_t *file = malloc(sizeof(file_t));
	strcpy(file->name, "Nombre del archivo de test");
	file->size = 1234;
	strcpy(file->parentId, "5554545454TESTTEST");

	mongo_file_save(file);

	printf("ID generated: %s\n", file->id);

	file_t *fileG = mongo_file_getById(file->id);

	printf("FILE CREATED:\n %s , size: %d\n", fileG->name, fileG->size);

	/*
	 * TODO.
	 const bson_t *docs[10];
	 docs = mongo_getAll();
	 int i;
	 for (i = 0; i < 10; i++) {
	 printf("FILE:\n %s\n", bson_as_json(docs[i], NULL));
	 }
	 */


	mongo_dir_shutdown();
	mongo_file_shutdown();
	mongo_shutdown();
}


void testCreateDir() {
	dir_t *dir = malloc(sizeof(dir_t));
	strcpy(dir->name, "usr");
	strcpy(dir->parentId, ROOT_DIR_ID);

	mongo_dir_save(dir);

	printf("ID generated: %s\n", dir->id);

	dir_t *dirG = mongo_dir_getById(dir->id);

	printf("DIR CREATED:\n %s \n", dirG->name);

	/*
	 * TODO.
	 const bson_t *docs[10];
	 docs = mongo_getAll();
	 int i;
	 for (i = 0; i < 10; i++) {
	 printf("FILE:\n %s\n", bson_as_json(docs[i], NULL));
	 }
	 */
}
