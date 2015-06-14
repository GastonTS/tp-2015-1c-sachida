#include <stdlib.h>
#include <string.h>
#include "job.h"

t_copy *CreateCopy(char *nodeName, uint16_t numBlock) {
	t_copy *copy = malloc(sizeof(t_copy));
	strcpy(copy->nodeName, nodeName);
	copy->numBlock = numBlock;
	return copy;
}

t_file *CreateFile(char *path) {
	t_file *file = malloc(sizeof(t_file));
	file->path = strdup(path);
	file->blocks = list_create();
	return file;
}

t_job *CreateJob(uint16_t id, bool combiner) {
	t_job *job = malloc(sizeof(t_job));
	job->id = id;
	job->combiner = combiner;
	job->files = list_create();
	job->finalReduce = malloc(sizeof(t_reduce));
	job->finalReduce->temps = list_create();
	job->partialReduces = list_create();
	job->maps = list_create();
	return job;
}

void freeCopies(t_list *copies) {
	list_destroy_and_destroy_elements(copies, (void *) free);
}

void freeFile(t_file *file) {
	free(file->path);
	list_destroy_and_destroy_elements(file->blocks, (void *) freeCopies);
	free(file);
}

void freeReduce(t_reduce *reduce) {
	list_destroy_and_destroy_elements(reduce->temps, (void *) free);
	free(reduce);
}

void freeJob(t_job *job) {
	list_destroy_and_destroy_elements(job->files, (void *) freeFile);
	list_destroy_and_destroy_elements(job->maps, (void *) free);
	list_destroy_and_destroy_elements(job->partialReduces, (void *) freeReduce);
	freeReduce(job->finalReduce);
	free(job);
}

bool isMap(t_map *map, uint16_t idMap) {
	return map->id == idMap;
}
