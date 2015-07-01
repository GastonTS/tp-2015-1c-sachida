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

void notificarMap(int jobSocket, t_map *map) {
	log_trace(logger, "Planned: %s", map->tempResultName);
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

int planMaps(t_job *job) {
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

				t_map *mapPlanned = CreateMap(list_size(job->maps), numBlock, selectedNode->port, selectedNode->name, selectedNode->ip, job->id);
				mapPlanned->copies = copies;
				list_add(job->maps, mapPlanned);
				notificarMap(job->socket, mapPlanned);
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
		sendDieOrder(job->socket);
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
	setTempMapName(map->tempResultName, map->id, job->id);

	notificarMap(job->socket, map);
	recvResult(job); //XXX test pendiente
}
