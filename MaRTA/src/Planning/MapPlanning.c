#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include "../structs/nodo.h"
#include "../MaRTA.h"
#include "MapPlanning.h"
#include <time.h>

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

void jobMap(t_job *job) {
	log_trace(logger, "Planning Job %d...", job->id);
	void fileMap(t_file *file) {
		void mapPlanning(t_list *copies) {
			t_node* selectedNode = NULL;
			int numBlock;

			void addNodeToAux(t_copy *copy) {
				bool lessWorkLoad(t_node *lessBusy, t_node *busy) {
					if (lessBusy && busy)
						return workLoad(lessBusy->maps, lessBusy->reduces) < workLoad(busy->maps, busy->reduces);
					return 0;
				}

				char* nodeName = copy->nodeName;

				bool nodeWithName(t_node *node) {
					return nodeByName(node, nodeName);
				}

				t_node *actualNode = (t_node*) list_find(nodes, (void*) nodeWithName);

				if (selectedNode == NULL || lessWorkLoad(actualNode, selectedNode)) {
					selectedNode = actualNode;
					numBlock = copy->numBlock;
				}
			}

			list_iterate(copies, (void*) addNodeToAux);
			list_add(selectedNode->maps, (void *) numBlock);

			t_map *mapPlanned = malloc(sizeof(t_map));
			mapPlanned->id = list_size(job->maps);
			mapPlanned->copies = copies;
			mapPlanned->nodeIP = selectedNode->ip;
			mapPlanned->nodePort = selectedNode->port;
			mapPlanned->numBlock = numBlock;
			char resultName[60] = "\"";
			strcat(resultName, getTime());
			strcat(resultName, "-Job(");
			char idJob[4];
			sprintf(idJob, "%i", job->id);
			strcat(resultName, idJob);
			strcat(resultName, ")-Map(");
			char numMap[4];
			sprintf(numMap, "%i", mapPlanned->id);
			strcat(resultName, numMap);
			strcat(resultName, ").txt\"");
			strcpy(mapPlanned->tempResultName, resultName);

			list_add(job->maps, mapPlanned);
			log_trace(logger, "\nMap planned: \n\tNode: %s \n\tBlock: %d \n\tStored in: %s", selectedNode->name, mapPlanned->numBlock,
					mapPlanned->tempResultName);
		}
		list_iterate(file->blocks, (void *) mapPlanning);
	}
	list_iterate(job->files, (void *) fileMap);
	log_trace(logger, "Finished Job %d...", job->id);
}
