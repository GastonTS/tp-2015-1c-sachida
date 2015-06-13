#ifndef SRC_STRUCTS_JOB_H_
#define SRC_STRUCTS_JOB_H_

#include <commons/collections/list.h>
#include <stdint.h>

typedef struct {
	char nodeName[25];
	uint32_t numBlock;
} t_copy;

typedef struct {
	uint32_t id;
	t_list *copies;
	char *nodeName;
	char *nodeIP;
	uint16_t nodePort;
	uint32_t numBlock;
	char tempResultName[60];
	bool done;
} t_map;

typedef struct {
	uint32_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char *tempName;
} t_temp;

typedef struct {
	t_list *temps;
	char *finalNode;
	char *nodeIP;
	uint16_t nodePort;
	char tempResultName[60];
	bool done;
} t_reduce;

typedef struct {
	char *path;
	t_list *blocks; //Cada elemento es una lista de t_bloque (copias)
} t_file;

typedef struct {
	uint32_t id;
	bool combiner;
	t_list *files;
	t_list *maps;
	t_list *partialReduces;
	t_reduce *finalReduce;
} t_job;

t_copy *CreateCopy(char *nodeName, uint32_t numBlock);
t_file *CreateFile(char *path);
t_job *CreateJob(uint32_t id, bool combiner);
void freeJob(t_job *job);
bool isMap(t_map *map, uint32_t idMap);

#endif
