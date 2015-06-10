#ifndef SRC_STRUCTS_NODO_H_
#define SRC_STRUCTS_NODO_H_

#include <commons/collections/list.h>

typedef struct {
	char name[25];
	char *ip;
	int port;
	int active;
	t_list *maps; //Los que esta llevando a cabo
	t_list *reduces; //Los que esta llevando a cabo
} t_node;

bool nodeByName(t_node *nodo, char *nombre);
int workLoad(t_list *maps, t_list *reduces);
void freeNode(t_node *nodo);
bool isActive(t_node *nodo);
t_node *findNode(t_list *nodes, char *nodeName);

#endif
