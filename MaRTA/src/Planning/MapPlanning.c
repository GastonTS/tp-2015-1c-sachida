#include <stdlib.h>
#include <string.h>
#include <commons/collections/list.h>
#include "../structs/job.h"
#include "../structs/nodo.h"
#include "../MaRTA.h"
#include "MapPlanning.h"
#include <time.h>

char* getTime() { //TODO:revisar si se puede ampliar a mili/microsegundos
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char * time = asctime(timeinfo);
	time[3] = '-';
	time[7] = '-';
	time[10] = '-';
	time[19] = '-';
	time[24] = '\0';

	return time;
}

map_t *mapPlanning(t_list *copias, t_list *nodos) {
	t_nodo* nodoSeleccionado = NULL;
	int nroBloque;

	void agregarNodoANodosAux(t_copia *copia) {
		bool menorCarga(t_nodo *noTanCargado, t_nodo *cargado) {
			if (noTanCargado && cargado)
				return cargaDeTrabajo(noTanCargado->maps, noTanCargado->reduces) < cargaDeTrabajo(cargado->maps, cargado->reduces);
			return 0;
		}

		char* nombreNodo = copia->nombreNodo;

		bool nodoConNombre(t_nodo *nodo) {
			return esNodo(nodo, nombreNodo);
		}

		t_nodo *nodoActual = (t_nodo*) list_find(nodos, (void*) nodoConNombre);

		if (nodoSeleccionado == NULL || menorCarga(nodoActual, nodoSeleccionado)) {
			nodoSeleccionado = nodoActual;
			nroBloque = copia->nroBloque;
		}

	}

	list_iterate(copias, (void*) agregarNodoANodosAux);
	list_add(nodoSeleccionado->maps, (void *) nroBloque);

	char nombreResultado[43] = "\"";
	strcat(nombreResultado, getTime());
	strcat(nombreResultado, "-Map-Bloque");
	char numBloque[4];
	sprintf(numBloque, "%i", nroBloque);
	strcat(nombreResultado, numBloque);
	strcat(nombreResultado, ".txt\"");

	map_t *mapPlanned = malloc(sizeof(map_t));
	mapPlanned->ipNodo = nodoSeleccionado->ipNodo;
	mapPlanned->puertoNodo = nodoSeleccionado->puertoNodo;
	mapPlanned->nroBloque = nroBloque;
	strcpy(mapPlanned->nombreResultado, nombreResultado);
	log_debug(logger, "Se planifico la tarea de map en el nodo %s y se va a almacenar en %s", nodoSeleccionado->nombreNodo, nombreResultado);
	return mapPlanned;
}
