#include "node.h"
#include <commons/string.h>

t_node *CreateNode(int active, char *IP, int port, char* name) {
	t_node *node = malloc(sizeof(t_node));
	node->active = active;
	node->ip = strdup(IP);
	node->port = port;
	node->name = strdup(name);
	node->maps = list_create();
	node->reduces = 0;
	return node;
}

bool nodeByName(t_node *node, char nombre[25]) {
	return string_equals_ignore_case(node->name, nombre);
}

int workLoad(t_list *maps, uint16_t reduces) {
	return list_size(maps) + reduces;
}

void freeNode(t_node *node) {
	if (node->ip) {
		free(node->ip);
	}
	if (node->name) {
		free(node->name);
	}
	list_destroy(node->maps);
	free(node);
}

bool isActive(t_node *node) {
	return node->active;
}

t_node *findNode(t_list *nodes, char *nodeName) {
	bool nodeWithName(t_node *node) {
		return nodeByName(node, nodeName);
	}

	return list_find(nodes, (void*) nodeWithName);
}

void showTasks(t_node *node) {
	printf("%s(Maps:%d-Reduces:%d)\n", node->name, list_size(node->maps), node->reduces);
}

void removeMapNode(t_map *map) {
	pthread_mutex_lock(&Mnodes);
	t_node *selectedNode = findNode(nodes, map->nodeName);
	bool isNumBlock(uint16_t numBlock) {
		return numBlock == map->numBlock;
	}
	list_remove_by_condition(selectedNode->maps, (void *) isNumBlock);
	pthread_mutex_unlock(&Mnodes);
}

void removeReduceNode(t_reduce *reduce) {
	pthread_mutex_lock(&Mnodes);
	t_node *node = findNode(nodes, reduce->finalNode);
	node->reduces--;
	pthread_mutex_unlock(&Mnodes);
}

void deactivateNode(char *nodeName) {
	t_node *node = findNode(nodes, nodeName);
	node->active = 0;
}
