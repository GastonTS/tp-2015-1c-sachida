#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>

bool canCreateResource(char *resourceName, char *parentId);
char* md5(char *str);

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

	void deleteChilds(char *parentId) {
		void deleteFile(file_t *file) {
			mongo_file_deleteById(file->id);
		}
		t_list *childFiles = filesystem_getFilesInDir(parentId);
		list_iterate(childFiles, (void*) deleteFile);
		list_destroy_and_destroy_elements(childFiles, (void*) file_free);

		void deleteDir(dir_t *dir) {
			deleteChilds(dir->id);
			mongo_dir_deleteById(dir->id);
		}
		t_list *childDirs = filesystem_getDirsInDir(parentId);
		list_iterate(childDirs, (void*) deleteDir);
		list_destroy_and_destroy_elements(childDirs, (void*) dir_free);
	}

	dir_t *dir = filesystem_getDirByNameInDir(dirName, parentId);
	if (dir) {
		deleteChilds(dir->id);
		bool r = mongo_dir_deleteById(dir->id);
		dir_free(dir);
		return r;
	} else {
		return 0;
	}
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

char* filesystem_md5sum(file_t* file) {
	// TODO
	return md5("asdasd");
}

// PRIVATE

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

char* md5(char *str) {
	FILE *md5pipe = NULL;
	size_t size = 9 + strlen(str) + 10 + 1;
	char *command = malloc(size);
	snprintf(command, size, "echo -n \"%s\" | md5sum", str);

	md5pipe = popen(command, "r");

	if (md5pipe != NULL) {
		char *buffer = malloc(sizeof(char) * 33);
		fread(buffer, 1, 32, md5pipe);

		pclose(md5pipe);
		free(command);

		return buffer;
	} else {
		free(command);
		return NULL;
	}
}
