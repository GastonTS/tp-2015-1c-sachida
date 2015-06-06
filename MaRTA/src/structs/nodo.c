#include <stdbool.h>
#include <string.h>
#include <commons/string.h>
#include "nodo.h"
#include <stdlib.h>

bool esNodo(t_nodo *nodo, char nombre[25]) {
	return string_equals_ignore_case(nodo->nombreNodo, nombre);
}

int cargaDeTrabajo(t_list *maps, t_list *reduces) {
	return list_size(maps) + list_size(reduces);
}

void freeNodo(t_nodo *nodo){
	list_destroy(nodo->maps);
	list_destroy(nodo->reduces);
	free(nodo);
}
