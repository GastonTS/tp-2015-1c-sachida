#include "MapPlanning.h"
#include "../structs/node.h"
#include "../Connections/Connection.h"

char* getTime() { //TODO:revisar si se puede ampliar a mili/microsegundos
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char * time = asctime(timeinfo);
	time[3] = '-';
	time[7] = '-';
	time[10] = '-';
	time[19] = '-';
	time[24] = '\0';

	return time;
}

void selectNode(t_copy *copy, t_node **selectedNode, uint16_t *numBlock) {
	bool lessWorkLoad(t_node *lessBusy, t_node *busy) {
		if (lessBusy && busy)
			return workLoad(lessBusy->maps, lessBusy->reduces) < workLoad(busy->maps, busy->reduces);
		return 0;
	}

	t_node *actualNode = findNode(nodes, copy->nodeName);

	if ((*selectedNode == NULL || lessWorkLoad(actualNode, *selectedNode)) && isActive(actualNode)) {
		*selectedNode = actualNode;
		*numBlock = copy->numBlock;
	}
}

void setTempMapName(t_map *map, t_job *job) {
	char resultName[60] = "\"";
	strcat(resultName, getTime());
	strcat(resultName, "-Job(");
	char idJob[4];
	sprintf(idJob, "%i", job->id);
	strcat(resultName, idJob);
	strcat(resultName, ")-Map(");
	char numMap[4];
	sprintf(numMap, "%i", map->id);
	strcat(resultName, numMap);
	strcat(resultName, ").txt\"");
	strcpy(map->tempResultName, resultName);
}

void notificarMap(int jobSocket, t_map *map) {
	log_trace(logger, "\nMap planned: \n\tIP Node: %s \n\tPort node: %d\n\tBlock: %d \n\tStored in: %s", map->nodeIP, map->nodePort, map->numBlock,
			map->tempResultName);
	map->done = false;
	serializeMapToOrder(jobSocket, map);
}

void removeMapNode(t_map *map) {
	t_node *selectedNode = findNode(nodes, map->nodeName);
	bool isNumBlock(uint16_t numBlock) {
		return numBlock == map->numBlock;
	}
	list_remove_by_condition(selectedNode->maps, (void *) isNumBlock);
}

int jobMap(t_job *job) {
	log_trace(logger, "Planning Job %d...", job->id);
	int filesAvailables = 1;
	void requestBlocks(t_file *file) {
		if (!requestFileBlocks(file))
			filesAvailables = 0;
	}
	list_iterate(job->files, (void *) requestBlocks);

	void fileMap(t_file *file) {
		void mapPlanning(t_list *copies) {
			t_node* selectedNode = NULL;
			uint16_t numBlock;

			void selectNodeToMap(t_copy *copy) {
				selectNode(copy, &selectedNode, &numBlock);
			}
			list_iterate(copies, (void*) selectNodeToMap);
			if (selectedNode == NULL) {
				log_info(logger, "File %s not available", file->path);
				list_iterate(job->maps, (void *) removeMapNode);
				filesAvailables = 0;
			} else {
				list_add(selectedNode->maps, &numBlock);

				t_map *mapPlanned = malloc(sizeof(t_map));
				mapPlanned->id = list_size(job->maps);
				mapPlanned->copies = copies;
				mapPlanned->nodeName = strdup(selectedNode->name);
				mapPlanned->nodeIP = strdup(selectedNode->ip);
				mapPlanned->nodePort = selectedNode->port;
				mapPlanned->numBlock = numBlock;
				setTempMapName(mapPlanned, job);
				list_add(job->maps, mapPlanned);
				notificarMap(job->jobSocket, mapPlanned);
			}
		}
		list_iterate(file->blocks, (void *) mapPlanning);
	}
	if (filesAvailables) {
		list_iterate(job->files, (void *) fileMap);
		log_trace(logger, "Finished Map Planning Job %d...", job->id);
		return EXIT_SUCCESS;
	} else {
		log_trace(logger, "Job %d Failed", job->id);
		return EXIT_FAILURE;
	}
}

void rePlanMap(t_job *job, t_map *map) {
	removeMapNode(map);
	t_node *selectedNode = NULL;
	uint16_t numBlock;

	void selectNodeToMap(t_copy *copy) {
		selectNode(copy, &selectedNode, &numBlock);
	}

	list_iterate(map->copies, (void*) selectNodeToMap);

	free(map->nodeName);
	map->nodeName = strdup(selectedNode->name);
	free(map->nodeIP);
	map->nodeIP = strdup(selectedNode->ip);
	map->nodePort = selectedNode->port;
	map->numBlock = numBlock;
	setTempMapName(map, job);

	notificarMap(job->jobSocket, map);
}
