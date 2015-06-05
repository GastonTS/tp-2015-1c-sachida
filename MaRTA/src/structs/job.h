#ifndef SRC_STRUCTS_JOB_H_
#define SRC_STRUCTS_JOB_H_


#include <commons/bitarray.h>

typedef struct {
	char nombreNodo[25];
	int nroBloque;
} t_copia;

typedef struct {
	char *nombreNodo;
	char *nombreTemp;
} t_temp;

typedef struct {
	t_list *temps;
	char *nodoDestino;
	char *temporalDestino;
} t_reduce;

typedef struct {
	char *path;
	t_list *bloques; //Cada elemento es una lista de t_bloque (copias)
	t_bitarray *maps;
} t_file;

typedef struct {
	char *nombre;
	t_list *files;
	t_reduce *reduces;
} t_job;

#endif
