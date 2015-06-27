#include <stdlib.h>
#include <string.h>
#include "job.h"

t_copy *CreateCopy(char *nodeName, uint16_t numBlock) {
	t_copy *copy = malloc(sizeof(t_copy));
	copy->nodeName = strdup(nodeName);
	copy->numBlock = numBlock;
	return copy;
}

t_map *CreateMap(uint16_t id, uint16_t numBlock, uint16_t nodePort, char *nodeName, char *nodeIP, char tempResultName[60]) {
	t_map *map = malloc(sizeof(t_map));
	map->id = id;
	map->numBlock = numBlock;
	map->nodePort = nodePort;
	map->nodeName = strdup(nodeName);
	map->nodeIP = strdup(nodeIP);
	memset(map->tempResultName, '\0', sizeof(char) * 60);
	strcpy(map->tempResultName, tempResultName);
	map->done = false;
	return map;
}

t_temp *CreateTemp(char *nodeIP, uint16_t nodePort, uint16_t originMap, char tempName[60]) {
	t_temp *temp = malloc(sizeof(t_temp));
	temp->nodeIP = strdup(nodeIP);
	temp->nodePort = nodePort;
	temp->originMap = originMap;
	memset(temp->tempName, '\0', sizeof(char) * 60);
	strcpy(temp->tempName, tempName);
	return temp;
}

t_file *CreateFile(char *path) {
	t_file *file = malloc(sizeof(t_file));
	file->path = strdup(path);
	file->blocks = list_create();
	return file;
}

t_reduce *CreateReduce() {
	t_reduce *reduce = malloc(sizeof(t_reduce));
	reduce->finalNode = NULL;
	reduce->nodeIP = NULL;
	reduce->temps = list_create();
	memset(reduce->tempResultName, '\0', sizeof(char) * 60);
	return reduce;
}

t_job *CreateJob(uint16_t id, bool combiner) {
	t_job *job = malloc(sizeof(t_job));
	job->id = id;
	job->combiner = combiner;
	job->files = list_create();
	job->finalReduce = CreateReduce();
	job->partialReduces = list_create();
	job->maps = list_create();
	return job;
}

void freeCopy(t_copy *copy) {
	if (copy->nodeName) { //FIXME si viene sin inicializar estallan los ifs
		free(copy->nodeName);
	}
	free(copy);
}

void freeCopies(t_list *copies) {
	list_destroy_and_destroy_elements(copies, (void *) freeCopy);
}

void freeFile(t_file *file) {
	if (file->path) {
		free(file->path);
	}

	list_destroy_and_destroy_elements(file->blocks, (void *) freeCopies);
	free(file);
}

void freeTemp(t_temp *temp) {
	if (temp->nodeIP) {
		free(temp->nodeIP);
	}
	free(temp);
}

void freeReduce(t_reduce *reduce) {
	if (reduce->finalNode) {
		free(reduce->finalNode);
	}
	if (reduce->nodeIP) {
		free(reduce->nodeIP);
	}
	list_destroy_and_destroy_elements(reduce->temps, (void *) freeTemp);
	free(reduce);
}

void freeMap(t_map *map) {
	if (map->nodeName) {
		free(map->nodeName);
	}
	if (map->nodeIP) {
		free(map->nodeIP);
	}
	free(map);
}

void freeJob(t_job *job) {
	list_destroy_and_destroy_elements(job->files, (void *) freeFile);
	list_destroy_and_destroy_elements(job->maps, (void *) freeMap);
	list_destroy_and_destroy_elements(job->partialReduces, (void *) freeReduce);
	freeReduce(job->finalReduce);
	free(job);
}

bool isMap(t_map *map, uint16_t idMap) {
	return map->id == idMap;
}

bool isReduce(t_reduce *reduce, uint16_t idReduce) {
	return reduce->id == idReduce;
}
