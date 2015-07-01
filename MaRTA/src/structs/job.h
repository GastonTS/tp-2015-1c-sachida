#ifndef SRC_STRUCTS_JOB_H_
#define SRC_STRUCTS_JOB_H_

#include "../MaRTA.h"

typedef struct {
	char* nodeName;
	uint16_t numBlock;
} t_copy;

typedef struct {
	uint16_t id;
	t_list *copies;
	char *nodeName;
	char *nodeIP;
	uint16_t nodePort;
	uint16_t numBlock;
	char tempResultName[60];
	bool done;
} t_map;

typedef struct {
	uint16_t originMap;
	char *nodeIP;
	uint16_t nodePort;
	char tempName[60];
} t_temp;

typedef struct {
	uint16_t id;
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
	int socket;
	uint16_t id;
	bool combiner;
	t_list *files;
	t_list *maps;
	t_list *partialReduces;
	t_reduce *finalReduce;
} t_job;

t_copy *CreateCopy(char *nodeName, uint16_t numBlock);
t_map *CreateMap(uint16_t id, uint16_t numBlock, uint16_t nodePort, char *nodeName, char *nodeIP, uint16_t jobID);
t_reduce *CreateReduce(uint16_t id, char *nodeName, char *nodeIP, uint16_t nodePort, uint16_t jobID);
void setFinalReduce(t_reduce *reduce, char *nodeName, char *nodeIP, uint16_t nodePort, uint16_t jobID);
t_temp *CreateTemp(char *nodeIP, uint16_t nodePort, uint16_t originMap, char tempName[60]);
t_file *CreateFile(char *path);
t_job *CreateJob(uint16_t id, bool combiner);
void setTempMapName(char tempMapName[60], uint16_t mapID, uint16_t jobID);
void setTempReduceName(char tempResultName[60], uint16_t jobID, char *tipo);
void freeMap(t_map* map);
void freeJob(t_job *job);
bool isMap(t_map *map, uint16_t idMap);
bool mapIsDone(t_map *map);
bool isReduce(t_reduce *reduce, uint16_t idReduce);

#endif
