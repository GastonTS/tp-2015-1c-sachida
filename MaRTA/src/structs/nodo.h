#ifndef SRC_STRUCTS_NODO_H_
#define SRC_STRUCTS_NODO_H_

typedef struct {
	int nroBloque;
} t_taskM;

typedef struct {
	char **nombresTemporales;
} t_taskR;

typedef struct {
	char *nombreNodo;
	char *ipNodo;
	int puertoNodo;
	char *estado;
	t_taskM *maps; //Los que esta llevando a cabo
	t_taskR *reduces; //Los que esta llevando a cabo
} t_nodo;

#endif
