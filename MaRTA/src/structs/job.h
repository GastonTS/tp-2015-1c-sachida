#ifndef SRC_STRUCTS_JOB_H_
#define SRC_STRUCTS_JOB_H_

#include <commons/collections/list.h>

typedef struct {
	char nodeName[25];
	int numBlock;
} t_copy;

typedef struct {
	int id;
	t_list *copies;
	char *nodeIP;
	int nodePort;
	int numBlock;
	char tempResultName[60];
} t_map;

typedef struct {
	char *nodeName;
	char *tempName;
} t_temp;

typedef struct {
	int originMap;
	t_list *temps;
	char *finalNode;
	char *tempResultName;
} t_reduce;

typedef struct {
	char *path;
	t_list *blocks; //Cada elemento es una lista de t_bloque (copias)
} t_file;

typedef struct {
	int id;
	t_list *files;
	t_list *maps;
	t_list *reduces;
} t_job;

void freeJob(t_job *job);

#endif
