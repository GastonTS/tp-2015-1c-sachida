#include <stdbool.h>
#include <commons/string.h>
#include "nodo.h"

bool esNodo(t_nodo nodo, char nombre[25]) {
	return string_equals_ignore_case(nodo.nombreNodo, nombre);
}

int cargaDeTrabajo(t_list *maps, t_list *reduces) {
	return list_size(maps) + list_size(reduces);
}
