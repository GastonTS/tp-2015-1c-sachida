#include "ReducePlanning.h"
#include "MapPlanning.h"
#include "../structs/job.h"
#include "../structs/node.h"
#include "../Connections/Connection.h"

typedef struct {
	t_node *node;
	uint16_t count;
} t_temporalCount;

void notificarReduce(t_job *job, t_reduce *reduce) {
	if (reduce->id)
		log_info(logger, "|JOB %d| Planned Partial Reduce: %d on Node: %s",
				job->id, reduce->id, reduce->finalNode);
	else
		log_info(logger, "|JOB %d| Planned Final Reduce: %d on Node: %s",
				job->id, reduce->id, reduce->finalNode);
	reduce->done = false;

	pthread_mutex_lock(&Mnodes);
	t_node *selectedNode = findNode(nodes, reduce->finalNode);
	selectedNode->reduces++;
	pthread_mutex_unlock(&Mnodes);

	if (0 > serializeReduceToOrder(job->socket, reduce)) {
		log_error(logger, "|JOB %d| Died when sending reduce order", job->id);
		freeJob(job);
		pthread_exit(NULL);
	}
}

t_temp * mapToTemporal(t_map *map) {
	t_temp *temporal = CreateTemp(map->nodeName, map->nodeIP, map->nodePort,
			map->id, map->tempResultName);
	return temporal;
}

t_temp * reduceToTemporal(t_reduce *reduce) {
	t_temp *temporal = CreateTemp(reduce->finalNode, reduce->nodeIP,
			reduce->nodePort, 0, reduce->tempResultName);
	return temporal;
}

int rePlanMapsFromNode(t_job *job, char *node) {
	int fileAvailable = 1;
	void rePlanByNode(t_map *map) {
		if (!strcmp(map->nodeName, node)) {
			job->mapsDone--;
			if (!rePlanMap(job, map))
				fileAvailable = 0;
		}
	}
	list_iterate(job->maps, (void *) rePlanByNode);
	free(node);
	return fileAvailable;
}

void noCombinerReducePlanning(t_job *job) {
	bool finalFailed;
	do {
		t_list *counts = list_create();

		pthread_mutex_lock(&Mnodes);
		void countTemporals(t_map *map) {
			bool findNodeInMaps(t_temporalCount *count) {
				return !strcmp(count->node->name, map->nodeName);
			}
			t_temporalCount *nodeCount = NULL;
			nodeCount = list_find(counts, (void*) findNodeInMaps);
			if (nodeCount == NULL) {
				nodeCount = malloc(sizeof(t_temporalCount));
				nodeCount->node = findNode(nodes, map->nodeName);
				nodeCount->count = 1;
				list_add(counts, (void *) nodeCount);
			} else
				nodeCount->count++;
		}
		list_iterate(job->maps, (void *) countTemporals);

		t_temporalCount *selectedCount = NULL;
		void selectMoreTempsNode(t_temporalCount *count) {
			if (selectedCount == NULL) {
				if (isActive(count->node))
					selectedCount = count;
			} else {
				if ((selectedCount->count < count->count)
						&& isActive(count->node))
					selectedCount = count;
			}
		}
		list_iterate(counts, (void *) selectMoreTempsNode);
		pthread_mutex_unlock(&Mnodes);

		if (selectedCount == NULL) {
			list_destroy_and_destroy_elements(counts, (void *) free);
			notifFileUnavailable(job);
		}

		setFinalReduce(job->finalReduce, selectedCount->node->name,
				selectedCount->node->ip, selectedCount->node->port, job->id);
		list_destroy_and_destroy_elements(counts, (void *) free);

		void createTemporal(t_map *map) {
			t_temp *temporal = mapToTemporal(map);
			list_add(job->finalReduce->temps, (void *) temporal);
		}

		list_iterate(job->maps, (void *) createTemporal);

		notificarReduce(job, job->finalReduce);
		finalFailed = false;
		char *fallenNode = recvResult(job);
		if (fallenNode == NULL) {
			bool finalResult = copyFinalTemporal(job);
			if (!finalResult)
				fallenNode = strdup(job->finalReduce->finalNode);
		}

		if (fallenNode != NULL) {
			finalFailed = true;
			if (!rePlanMapsFromNode(job, fallenNode))
				notifFileUnavailable(job);
			list_clean_and_destroy_elements(job->finalReduce->temps,
					(void *) freeTemp);
			free(job->finalReduce->finalNode);
			free(job->finalReduce->nodeIP);
		}
	} while (finalFailed);

}

void combinerPartialsReducePlanning(t_job *job) {
	void agregarAPartialReduces(t_map *map) {
		t_temp *temporal = mapToTemporal(map);
		bool findReduce(t_reduce *reduce) {
			return !strcmp(reduce->finalNode, map->nodeName);
		}
		t_reduce *reduce = list_find(job->partialReduces, (void *) findReduce);
		if (reduce == NULL) {
			reduce = CreateReduce((list_size(job->partialReduces) + 1),
					map->nodeName, map->nodeIP, map->nodePort, job->id);
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
			return workLoad(lessBusy->maps, lessBusy->reduces)
					< workLoad(busy->maps, busy->reduces);
		return 0;
	}

	t_node *actualNode = findNode(nodes, reduce->finalNode);

	if ((*selectedNode == NULL || lessWorkLoad(actualNode, *selectedNode))
			&& isActive(actualNode)) {
		*selectedNode = actualNode;
	}
}

void combinerFinalReducePlanning(t_job *job) {
	t_node *selectedNode = NULL;

	pthread_mutex_lock(&Mnodes);
	void selectFinalNode(t_reduce *reduce) {
		searchNode(reduce, &selectedNode);
	}
	list_iterate(job->partialReduces, (void *) selectFinalNode);
	setFinalReduce(job->finalReduce, selectedNode->name, selectedNode->ip,
			selectedNode->port, job->id);
	pthread_mutex_unlock(&Mnodes);

	void createFinalTemporals(t_reduce *reduce) {
		t_temp *temporal = reduceToTemporal(reduce);
		list_add(job->finalReduce->temps, (void *) temporal);
	}
	list_iterate(job->partialReduces, (void *) createFinalTemporals);

	notificarReduce(job, job->finalReduce);
}

void combinerReducePlanning(t_job *job) {
	bool partialsFailed;
	bool finalFailed;
	t_list *fallenNodes = list_create();
	char *fallenNode;
	do {
		finalFailed = false;
		do {
			partialsFailed = false;
			combinerPartialsReducePlanning(job);
			int i;
			int reduceCount = list_size(job->partialReduces);
			for (i = 0; i < reduceCount; i++) {
				fallenNode = recvResult(job);
				if (fallenNode != NULL) {
					partialsFailed = true;
					list_add(fallenNodes, fallenNode);
				}
			}
			int filesAvailables = 1;
			if (partialsFailed) {
				void replanMaps(char *node) {
					if (!rePlanMapsFromNode(job, node))
						filesAvailables = 0;
				}
				list_iterate(fallenNodes, (void *) replanMaps);
				if (!filesAvailables) {
					list_destroy(fallenNodes);
					notifFileUnavailable(job);
				}
				list_clean(fallenNodes);
				list_clean_and_destroy_elements(job->partialReduces,
						(void *) freeReduce);
			}
		} while (partialsFailed);
		list_clean(fallenNodes);
		combinerFinalReducePlanning(job);
		fallenNode = recvResult(job);
		if (fallenNode == NULL) {
			bool finalResult = copyFinalTemporal(job);
			if (!finalResult)
				fallenNode = strdup(job->finalReduce->finalNode);
		}
		if (fallenNode != NULL) {
			finalFailed = true;
			if (!rePlanMapsFromNode(job, fallenNode)) {
				list_destroy(fallenNodes);
				notifFileUnavailable(job);
			}
			list_clean_and_destroy_elements(job->partialReduces,
					(void *) freeReduce);
			list_clean_and_destroy_elements(job->finalReduce->temps,
					(void *) freeTemp);
		}
	} while (finalFailed);
}

