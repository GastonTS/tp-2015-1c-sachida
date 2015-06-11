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
	char *nodeName;
	char *nodeIP;
	int nodePort;
	int numBlock;
	char tempResultName[60];
	bool done;
} t_map;

typedef struct {
	int originMap;
	char *nodeIP;
	int nodePort;
	char *tempName;
} t_temp;

typedef struct {
	t_list *temps;
	char *finalNode;
	char *nodeIP;
	int nodePort;
	char tempResultName[60];
	bool done;
} t_reduce;

typedef struct {
	char *path;
	t_list *blocks; //Cada elemento es una lista de t_bloque (copias)
} t_file;

typedef struct {
	int id;
	bool combiner;
	t_list *files;
	t_list *maps;
	t_list *partialReduces;
	t_reduce *finalReduce;
} t_job;

void freeJob(t_job *job);
bool isMap(t_map *map, int idMap);

#endif
