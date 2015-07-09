#include "MapPlanning.h"
#include "../structs/node.h"
#include "../Connections/Connection.h"

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

void notificarMap(t_job *job, t_map *map) {
	log_info(logger, "|JOB %d| Planned Map: %d on Node: %s (Block:%d)", job->id, map->id, map->nodeName, map->numBlock);
	map->done = false;
	if (0 > serializeMapToOrder(job->socket, map)) {
		log_error(logger, "Job %d Died when sending map order", job->id);
		freeJob(job);
		pthread_exit(NULL);
	}
}

void planMaps(t_job *job) {
	log_info(logger, "Planning Job %d...", job->id);
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
			pthread_mutex_lock(&Mnodes);
			list_iterate(copies, (void*) selectNodeToMap);
			pthread_mutex_unlock(&Mnodes);

			if (selectedNode == NULL) {
				log_info(logger, "File %s not available", file->path);
				list_iterate(job->maps, (void *) removeMapNode);
				filesAvailables = 0;
			} else {
				list_add(selectedNode->maps, (void *) (intptr_t) numBlock);
				t_map *mapPlanned = CreateMap(list_size(job->maps), numBlock, selectedNode->port, selectedNode->name, selectedNode->ip, job->id);
				mapPlanned->copies = copies;
				list_add(job->maps, mapPlanned);
				notificarMap(job, mapPlanned);
			}
		}
		list_iterate(file->blocks, (void *) mapPlanning);
	}
	if (filesAvailables) {
		list_iterate(job->files, (void *) fileMap);
		log_info(logger, "Finished Map Planning Job %d", job->id);
		log_info(logger, "Waiting Map Results from Job %d...", job->id);
		int i;
		int mapsCount = list_size(job->maps);
		for (i = 0; i < mapsCount; i++)
			recvResult(job);
	} else {
		log_error(logger, "Job %d Failed: One file is unavailable", job->id);
		sendDieOrder(job->socket, COMMAND_RESULT_FILEUNAVAILABLE);
		freeJob(job);
		pthread_exit(0);
	}
}

void rePlanMap(t_job *job, t_map *map) {
	t_node *selectedNode = NULL;
	uint16_t numBlock;

	void selectNodeToMap(t_copy *copy) {
		selectNode(copy, &selectedNode, &numBlock);
	}

	pthread_mutex_lock(&Mnodes);
	list_iterate(map->copies, (void*) selectNodeToMap);
	pthread_mutex_unlock(&Mnodes);

	if (selectedNode != NULL) {
		free(map->nodeName);
		map->nodeName = strdup(selectedNode->name);
		free(map->nodeIP);
		map->nodeIP = strdup(selectedNode->ip);
		map->nodePort = selectedNode->port;
		map->numBlock = numBlock;
		setTempMapName(map->tempResultName, map->id, job->id);

		notificarMap(job, map);
		recvResult(job);
	} else {
		log_error(logger, "Job %d Failed: One file is unavailable", job->id);
		sendDieOrder(job->socket, COMMAND_RESULT_FILEUNAVAILABLE);
		freeJob(job);
		pthread_exit(0);
	}
}
