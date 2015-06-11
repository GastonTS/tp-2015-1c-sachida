#include <stdlib.h>
#include "job.h"

void freeCopies(t_list *copies) {
	list_destroy_and_destroy_elements(copies, (void *) free);
}

void freeFile(t_file *file) {
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

bool isMap(t_map *map, int idMap) {
	return map->id == idMap;
}
