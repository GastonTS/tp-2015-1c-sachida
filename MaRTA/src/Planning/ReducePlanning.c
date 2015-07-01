#include "ReducePlanning.h"
#include "../structs/job.h"
#include "../structs/node.h"
#include "../Connections/Connection.h"

typedef struct {
	char *nodeName;
	uint16_t count;
} t_temporalCount;

void notificarReduce(t_job *job, t_reduce *reduce) {
	log_trace(logger, "Planned: %s", reduce->tempResultName);
	reduce->done = false;
	t_node *selectedNode = findNode(nodes, reduce->finalNode);
	list_add(selectedNode->reduces, (void *) reduce->temps);
	if (0 > serializeReduceToOrder(job->socket, reduce)) {
		log_error(logger, "Job %d Died when sending reduce order", job->id);
		freeJob(job);
		pthread_exit(NULL);
	}
}

t_temp * mapToTemporal(t_map *map) {
	t_temp *temporal = CreateTemp(map->nodeIP, map->nodePort, map->id, map->tempResultName);
	return temporal;
}

t_temp * reduceToTemporal(t_reduce *reduce) {
	t_temp *temporal = CreateTemp(reduce->nodeIP, reduce->nodePort, 0, reduce->tempResultName);
	return temporal;
}

void noCombinerReducePlanning(t_job *job) {
	t_list *counts = list_create();
	void countTemporals(t_map *map) {
		bool findNodeInMaps(t_temporalCount *count) {
			return !strcmp(count->nodeName, map->nodeName);
		}
		t_temporalCount *nodeCount = NULL;
		nodeCount = list_find(counts, (void*) findNodeInMaps);
		if (nodeCount == NULL) {
			nodeCount = malloc(sizeof(t_temporalCount));
			nodeCount->nodeName = strdup(map->nodeName);
			nodeCount->count = 1;
			list_add(counts, (void *) nodeCount);
		} else
			nodeCount->count++;
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
	void freeCounts(t_temporalCount *count) {
		if (count->nodeName) {
			free(count->nodeName);
		}
		free(count);
	}
	list_destroy_and_destroy_elements(counts, (void *) freeCounts);

	setFinalReduce(job->finalReduce, selectedNode->name, selectedNode->ip, selectedNode->port, job->id);

	void createTemporal(t_map *map) {
		t_temp *temporal = mapToTemporal(map);
		list_add(job->finalReduce->temps, (void *) temporal);
	}

	list_iterate(job->maps, (void *) createTemporal);

	notificarReduce(job, job->finalReduce);
}

void combinerPartialsReducePlanning(t_job *job) {
	void agregarAPartialReduces(t_map *map) {
		t_temp *temporal = mapToTemporal(map);
		bool findReduce(t_reduce *reduce) {
			return !strcmp(reduce->finalNode, map->nodeName);
		}
		t_reduce *reduce = list_find(job->partialReduces, (void *) findReduce);
		if (reduce == NULL) {
			reduce = CreateReduce((list_size(job->partialReduces) + 1), map->nodeName, map->nodeIP, map->nodePort, job->id);
			list_add(reduce->temps, temporal);
			list_add(job->partialReduces, reduce);
		} else
			list_add(reduce->temps, temporal);
	}
	list_iterate(job->maps, (void *) agregarAPartialReduces);

	void notificar(t_reduce *reduce) {
		notificarReduce(job, reduce);
	}
	list_iterate(job->partialReduces, (void *) notificar);
}

void searchNode(t_reduce *reduce, t_node **selectedNode) {
	bool lessWorkLoad(t_node *lessBusy, t_node *busy) {
		if (lessBusy && busy)
			return workLoad(lessBusy->maps, lessBusy->reduces) < workLoad(busy->maps, busy->reduces);
		return 0;
	}

	t_node *actualNode = findNode(nodes, reduce->finalNode);

	if ((*selectedNode == NULL || lessWorkLoad(actualNode, *selectedNode)) && isActive(actualNode)) {
		*selectedNode = actualNode;
	}
}

void combinerFinalReducePlanning(t_job *job) {
	t_node *selectedNode = NULL;

	void selectFinalNode(t_reduce *reduce) {
		searchNode(reduce, &selectedNode);
	}

	list_iterate(job->partialReduces, (void *) selectFinalNode);
	setFinalReduce(job->finalReduce, selectedNode->name, selectedNode->ip, selectedNode->port, job->id);

	void createTemporal(t_reduce *reduce) {
		t_temp *temporal = reduceToTemporal(reduce);
		list_add(job->finalReduce->temps, (void *) temporal);
	}
	list_iterate(job->partialReduces, (void *) createTemporal);

	notificarReduce(job, job->finalReduce);
}
