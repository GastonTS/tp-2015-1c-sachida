#ifndef SRC_PLANNING_MAPPLANNING_H_
#define SRC_PLANNING_MAPPLANNING_H_

typedef struct{
	char *ipNodo;
	int puertoNodo;
	int nroBloque;
	char nombreResultado[43];
}map_t;

map_t *mapPlanning(t_list *copias, t_list *nodos);

#endif
