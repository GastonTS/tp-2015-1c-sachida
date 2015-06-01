#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>

bool canCreateResource(char *resourceName, char *parentId);

t_log* logger;

dir_t* filesystem_getDirById(char *id) {
	return mongo_dir_getById(id);
}

dir_t* filesystem_getDirByNameInDir(char *dirName, char *parentId) {
	return mongo_dir_getByNameInDir(dirName, parentId);
}

file_t* filesystem_getFileByNameInDir(char *fileName, char *parentId) {
	return mongo_file_getByNameInDir(fileName, parentId);
}

bool filesystem_format() {
	return mongo_dir_deleteAll() && mongo_file_deleteAll();
}

bool filesystem_deleteDirByNameInDir(char *dirName, char *parentId) {
	return mongo_dir_deleteDirByNameInDir(dirName, parentId);

	// TODO, recursively delete files and dirs under this dir.
	// TODO, delete files? and folders recursively. do here? add service layer.
}

bool filesystem_deleteFileByNameInDir(char *fileName, char *parentId) {
	return mongo_file_deleteFileByNameInDir(fileName, parentId);
}

void filesystem_moveFile(file_t *file, char *destinationId) {
	mongo_file_updateParentId(file->id, destinationId);
}

void filesystem_moveDir(dir_t *dir, char *destinationId) {
	mongo_dir_updateParentId(dir->id, destinationId);
}

bool filesystem_addFile(file_t *file) {
	if (!canCreateResource(file->name, file->parentId)) {
		return 0;
	}

	return !mongo_file_save(file);
}

bool filesystem_addDir(dir_t *dir) {
	if (!canCreateResource(dir->name, dir->parentId)) {
		return 0;
	}

	return !mongo_dir_save(dir);
}

t_list* filesystem_getDirsInDir(char *parentId) {
	return mongo_dir_getByParentId(parentId);
}

t_list* filesystem_getFilesInDir(char *parentId) {
	return mongo_file_getByParentId(parentId);
}

node_t* filesystem_getNodeByName(char *nodeName) {
	return mongo_node_getByName(nodeName);
}

// Private

bool canCreateResource(char *resourceName, char *parentId) {
	dir_t *dir = filesystem_getDirByNameInDir(resourceName, parentId);
	if (dir) {
		dir_free(dir);
		return 0;
	}

	file_t *file = filesystem_getFileByNameInDir(resourceName, parentId);
	if (file) {
		file_free(file);
		return 0;
	}

	return 1;
}

// FUNCTIONS ..

/*

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
 // TODO armar bien esto..
 printf("MD5 de %s\n", file);
 char *input = strdup("asdasd");

 md5(input);

 free(input);
 }
 }

 void md5(char *str) {
 FILE *md5pipe = NULL;
 size_t size = 9 + strlen(str) + 10 + 1;
 char *command = malloc(size);
 snprintf(command, size, "echo -n \"%s\" | md5sum", str);

 md5pipe = popen(command, "r");

 if (md5pipe != NULL) {
 char buffer[32];
 fread(buffer, 1, 32, md5pipe);
 printf("%s\n", buffer);

 pclose(md5pipe);
 free(command);
 } else {
 printf("No se pudo obtener el md5.\n");
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
 while (dirNames[i]) {
 file = dirNames[i];
 i++;
 }
 printf("Copia el archivo %s al MDFS: %s\n", file, dest);
 readFile(source);
 freeSplits(dirNames);
 } else if (string_equals_ignore_case(option, "-tofs")) {
 printf("Copia el archivo %s al FS: %s\n", source, dest);
 } else {
 printf("Invalid option %s \n", option);
 }
 }
 }

 void printNodeStatus(char *nodeName) {
 if (!isNull(nodeName)) {
 node_t *node = mongo_node_getByName(nodeName);

 if (node) {
 node_printBlocksStatus(node);
 node_free(node);
 } else {
 printf("No existe el nodo %s\n", nodeName);
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

 t_list* getFileBlocks(char *route) {
 int blockSize = 20 * 1024 * 1024; // 20 MB.

 FILE *fp;
 char *line = NULL;
 ssize_t linesize;
 size_t len = 0;

 int bytesRead = 0;
 char *buffer = malloc(sizeof(char) * blockSize);
 t_list *blocks = list_create();
 strcpy(buffer, "");

 fp = fopen(route, "r");
 if (fp == NULL) {
 printf("Local file %s not found\n", route);
 return NULL;
 }

 while ((linesize = getline(&line, &len, fp)) != -1) {
 if (bytesRead + linesize > blockSize) {
 printf("New block");
 list_add(blocks, buffer);
 // TODO, fill the rest with \0 .. and other stuff.input[strlen(input) - 1] = '\0';

 // Reset all data for the next buffer.
 buffer = malloc(sizeof(char) * blockSize);
 strcpy(buffer, line);
 bytesRead = linesize;
 } else {
 //printf("%d\%   \n", bytesRead * 100 / blockSize);
 strcat(buffer, line);
 bytesRead += linesize;
 }
 }
 list_add(blocks, buffer);

 //free's
 fclose(fp);
 if (line) {
 free(line);
 }

 return blocks;
 }

 // TODO move:
 void readFile(char *route) {
 t_list *blocks = getFileBlocks(route);

 printf("List size: %d \n", list_size(blocks));
 void printBlockSize(char *buffer) {
 printf("Buffer size: %d \n", strlen(buffer));
 }
 list_iterate(blocks, (void *) printBlockSize);
 printf("MD5:");

 char *fileBuffer = strdup("");
 void concatBuffers(char *buffer) {
 fileBuffer = realloc(fileBuffer, sizeof(char) * (strlen(buffer) + strlen(fileBuffer) + 1));
 strcat(fileBuffer, buffer);
 }
 list_iterate(blocks, (void *) concatBuffers);
 md5(fileBuffer);

 list_destroy_and_destroy_elements(blocks, free);
 }



 */
