#include "../structs/job.h"
#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include "../structs/nodo.h"
#include "../MaRTA.h"
#include "MapPlanning.h"
#include <time.h>

typedef struct {
	char *nodeName;
	int count;
} t_temporalCount;

void notificarReduce(t_reduce *reduce) {
	log_trace(logger, "\nReduce planned: \n\tIP Node: %s \n\tPort node: %d\n\tStored in: %s", reduce->nodeIP, reduce->nodePort,
			reduce->tempResultName);
	reduce->done = false;
	//TODO: enviar reduce al proceso job.
}

void setTempReduceName(char tempResultName[60], t_job *job, char *tipo) {
	char resultName[60] = "\"";
	strcat(resultName, getTime());
	strcat(resultName, "-Job(");
	char idJob[4];
	sprintf(idJob, "%i", job->id);
	strcat(resultName, idJob);
	strcat(resultName, ")-Red(");
	strcat(resultName, tipo);
	strcat(resultName, ").txt\"");
	strcpy(tempResultName, resultName);
}

void reducePlanning(t_job *job) {
	if (!(job->combiner)) {
		t_list *counts = list_create();
		void countTemporals(t_map *map) {
			bool findNode(t_temporalCount *count) {
				return !strcmp(count->nodeName, map->nodeName);
			}
			t_temporalCount *node = NULL;
			node = list_find(counts, (void*) findNode);
			if (node == NULL) {
				node = malloc(sizeof(t_temporalCount));
				node->nodeName = map->nodeName;
				node->count = 1;
				list_add(counts, (void *) node);
			} else
				node->count++;
		}
		list_iterate(job->maps, (void *) countTemporals);

		t_temporalCount *selectedCount = NULL;
		void selectMoreTempsNode(t_temporalCount *count) {
			if (selectedCount == NULL) {
				selectedCount = count;
			} else {
				if (selectedCount->count < count->count)
					selectedCount = count;
			}
		}

		list_iterate(counts, (void *) selectMoreTempsNode);
		t_node *selectedNode = findNode(nodes, selectedCount->nodeName);
		list_destroy_and_destroy_elements(counts, (void *) free);

		t_list *finalTemps = list_create();
		void createTemporal(t_map *map) {
			t_temp *temporal = malloc(sizeof(t_temp));
			temporal->originMap = map->id;
			temporal->nodeIP = map->nodeIP;
			temporal->nodePort = map->nodePort;
			temporal->tempName = map->tempResultName;
			list_add(finalTemps, (void *) temporal);
		}
		list_iterate(job->maps, (void *) createTemporal);
		t_reduce *finalReduce = malloc(sizeof(t_reduce));
		finalReduce->finalNode = selectedNode->name;
		finalReduce->nodeIP = selectedNode->ip;
		finalReduce->nodePort = selectedNode->port;
		finalReduce->temps = finalTemps;
		finalReduce->done = 0;
		setTempReduceName(finalReduce->tempResultName, job, "Fin");
		notificarReduce(finalReduce);
	} else {
		//TODO Planificar combiner
	}
}

