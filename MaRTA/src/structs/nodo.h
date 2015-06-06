#ifndef SRC_STRUCTS_NODO_H_
#define SRC_STRUCTS_NODO_H_

#include <commons/collections/list.h>

typedef struct {
	char nombreNodo[25];
	char *ipNodo;
	int puertoNodo;
	char *estado;
	t_list *maps; //Los que esta llevando a cabo
	t_list *reduces; //Los que esta llevando a cabo
} t_nodo;

bool esNodo(t_nodo *nodo, char *nombre);
int cargaDeTrabajo(t_list *maps, t_list *reduces);
void freeNodo(t_nodo *nodo);

#endif
