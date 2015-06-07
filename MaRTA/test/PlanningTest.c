#include <string.h>
#include <stdlib.h>
#include <commons/collections/list.h>
#include "../src/MaRTA.h"
#include "../src/structs/nodo.h"
#include "../src/structs/job.h"
#include "../src/Planning/MapPlanning.h"

void mapPlanningtest() {
	t_nodo* nodo1 = malloc(sizeof(t_nodo));
	strcpy(nodo1->nombreNodo, "Nodo1");
	nodo1->maps = list_create();
	list_add(nodo1->maps, (void *) 1);
	list_add(nodo1->maps, (void *) 1);
	list_add(nodo1->maps, (void *) 1);
	list_add(nodo1->maps, (void *) 1);
	nodo1->reduces = list_create();
	list_add(nodo1->reduces, (void *) "datos.txt");

	t_nodo* nodo2 = malloc(sizeof(t_nodo));
	strcpy(nodo2->nombreNodo, "Nodo2");
	nodo2->maps = list_create();
	list_add(nodo2->maps, (void *) 1);
	nodo2->reduces = list_create();
	list_add(nodo2->reduces, (void *) "datos.txt");

	t_nodo* nodo3 = malloc(sizeof(t_nodo));
	strcpy(nodo3->nombreNodo, "Nodo3");
	nodo3->maps = list_create();
	list_add(nodo3->maps, (void *) 1);
	nodo3->reduces = list_create();
	list_add(nodo3->reduces, (void *) "datos.txt");
	list_add(nodo3->reduces, (void *) "datos.txt");
	list_add(nodo3->reduces, (void *) "datos.txt");

	t_nodo* nodo4 = malloc(sizeof(t_nodo));
	strcpy(nodo4->nombreNodo, "Nodo4");
	nodo4->maps = list_create();
	nodo4->reduces = list_create();

	t_nodo* nodo5 = malloc(sizeof(t_nodo));
	strcpy(nodo5->nombreNodo, "Nodo5");
	nodo5->maps = list_create();
	nodo5->reduces = list_create();

	list_add(nodos, nodo1);
	list_add(nodos, nodo2);
	list_add(nodos, nodo3);
	list_add(nodos, nodo4);
	list_add(nodos, nodo5);

	t_copia *copia1 = malloc(sizeof(t_copia));
	strcpy(copia1->nombreNodo, "Nodo1");
	copia1->nroBloque = 1;

	t_copia *copia2 = malloc(sizeof(t_copia));
	strcpy(copia2->nombreNodo, "Nodo2");
	copia2->nroBloque = 1;

	t_copia *copia3 = malloc(sizeof(t_copia));
	strcpy(copia3->nombreNodo, "Nodo5");
	copia3->nroBloque = 17;

	t_list *copias = list_create();
	list_add(copias, (void *) copia1);
	list_add(copias, (void *) copia2);
	list_add(copias, (void *) copia3);

	t_copia *copia2_1 = malloc(sizeof(t_copia));
	strcpy(copia2_1->nombreNodo, "Nodo1");
	copia2_1->nroBloque = 1;

	t_copia *copia2_2 = malloc(sizeof(t_copia));
	strcpy(copia2_2->nombreNodo, "Nodo5");
	copia2_2->nroBloque = 1;

	t_copia *copia2_3 = malloc(sizeof(t_copia));
	strcpy(copia2_3->nombreNodo, "Nodo4");
	copia2_3->nroBloque = 1;

	t_list *copias2 = list_create();
	list_add(copias2, (void *) copia2_1);
	list_add(copias2, (void *) copia2_2);
	list_add(copias2, (void *) copia2_3);

	map_t *map1;
	map_t *map2;
	map1 = mapPlanning(copias, nodos);
	map2 = mapPlanning(copias2, nodos);
	free(map1);
	free(map2);

	list_destroy_and_destroy_elements(nodos, (void *) freeNodo);
	list_destroy_and_destroy_elements(copias, (void *) free);
	list_destroy_and_destroy_elements(copias2, (void *) free);
}
