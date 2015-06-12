#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <commons/string.h>
#include <stdlib.h>
#include "node.h"

t_node *CreateNode(int active, char *IP, int port, char name[25]) {
	t_node *node = malloc(sizeof(t_node));
	node->active = active;
	node->ip = IP;
	node->port = port;
	strcpy(node->name, name);
	node->maps = list_create();
	node->reduces = list_create();
	return node;
}

bool nodeByName(t_node *node, char nombre[25]) {
	return string_equals_ignore_case(node->name, nombre);
}

int workLoad(t_list *maps, t_list *reduces) {
	return list_size(maps) + list_size(reduces);
}

void freeNode(t_node *node) {
	list_destroy(node->maps);
	list_destroy(node->reduces);
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
	printf("%s(Maps:%d-Reduces:%d)\n", node->name, list_size(node->maps), list_size(node->reduces));
}
