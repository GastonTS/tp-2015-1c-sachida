#ifndef SRC_STRUCTS_JOB_H_
#define SRC_STRUCTS_JOB_H_

typedef struct {
	char *nombreNodo;
	int nroBloque;
} t_bloque;

typedef struct {
	char *nombreNodo;
	char *nombreTemp;
} t_temp;

typedef struct {
	t_temp *temps;
	char *nodoDestino;
	char *temporalDestino;
} t_reduce;

typedef struct {
	char *path;
	t_bloque **bloques;
	t_bitarray *maps;
} t_file;

typedef struct {
	char *nombre;
	t_file *files;
	t_reduce *reduces;
} t_job;

#endif
