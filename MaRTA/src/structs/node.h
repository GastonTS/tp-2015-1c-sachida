#ifndef SRC_STRUCTS_NODE_H_
#define SRC_STRUCTS_NODE_H_

#include "../MaRTA.h"
#include "job.h"

typedef struct {
	char *name;
	char *ip;
	int port;
	int active;
	t_list *maps; //Los que esta llevando a cabo
	uint16_t reduces; //solo para saber la cantidad podria identificarse unequivcamente con el nombre del tempoal
} t_node;

bool nodeByName(t_node *nodo, char *nombre);
int workLoad(t_list *maps, uint16_t reduces);
void freeNode(t_node *nodo);
bool isActive(t_node *nodo);
t_node *findNode(t_list *nodes, char *nodeName);
t_node *CreateNode(int active, char *IP, int port, char name[25]);
void showTasks(t_node *node);
void removeMapNode(t_map *map);
void removeReduceNode(t_reduce *reduce);

#endif
