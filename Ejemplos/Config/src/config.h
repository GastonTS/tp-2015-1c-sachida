#ifndef CFG_H_ //Si NO esta esta label definida
#define CFG_H_ //La defino y copio el hedear con el codigo

typedef struct{ //Puse 4, 2 para obtener la property, 2 para probar el default
	char *IP;
	int PUERTO;
	char *IP2;
	int PUERTO2;
}t_configuracion;

extern t_configuracion* CONFIG; //extern indica que lo voy a ver en cualquier lado que linkee el .h

void initConfig();
void freeConfig();

#endif
