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
	log_trace(logger, "\nReduce planned: \n\tIP Node: %s \n\tPort node: %d\n\tStored in: %s", reduce->nodeIP, reduce->nodePort, reduce->tempResultName);
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

t_temp * mapToTemporal(t_map *map) {
	t_temp *temporal = malloc(sizeof(t_temp));
	temporal->originMap = map->id;
	temporal->nodeIP = map->nodeIP;
	temporal->nodePort = map->nodePort;
	temporal->tempName = map->tempResultName;
	return temporal;
}

void noCombinerReducePlanning(t_job *job) {
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

	void createTemporal(t_map *map) {
		t_temp *temporal = mapToTemporal(map);
		list_add(job->finalReduce->temps, (void *) temporal);
	}

	list_iterate(job->maps, (void *) createTemporal);
	job->finalReduce->finalNode = selectedNode->name;
	job->finalReduce->nodeIP = selectedNode->ip;
	job->finalReduce->nodePort = selectedNode->port;
	job->finalReduce->done = 0;
	setTempReduceName(job->finalReduce->tempResultName, job, "Fin");
}

void combinerPartialsReducePlanning(t_job *job) {
	void agregarAPartialReduces(t_map *map) {
		t_temp *temporal = mapToTemporal(map);
		bool findReduce(t_reduce *reduce) {
			return !strcmp(reduce->finalNode, map->nodeName);
		}
		t_reduce *reduce = list_find(job->partialReduces, (void *) findReduce);
		if (reduce == NULL) {
			reduce = malloc(sizeof(t_reduce));
			reduce->finalNode = map->nodeName;
			reduce->nodeIP = map->nodeIP;
			reduce->nodePort = map->nodePort;
			reduce->temps = list_create();
			list_add(reduce->temps, temporal);
			setTempReduceName(reduce->tempResultName, job, "Par");
			list_add(job->partialReduces, reduce);
		} else
			list_add(reduce->temps, temporal);
	}
	list_iterate(job->maps, (void *) agregarAPartialReduces);

	list_iterate(job->partialReduces, (void *) notificarReduce);
}
