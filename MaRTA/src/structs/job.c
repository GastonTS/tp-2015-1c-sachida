#include <stdlib.h>
#include "job.h"

void freeCopies(t_list *copies) {
	list_destroy_and_destroy_elements(copies, (void *) free);
}

void freeFile(t_file *file) {
	list_destroy_and_destroy_elements(file->blocks, (void *) freeCopies);
	free(file);
}

void freeJob(t_job *job) {
	list_destroy_and_destroy_elements(job->files, (void *) freeFile);
	list_destroy_and_destroy_elements(job->maps, (void *) free);
	free(job);
}
