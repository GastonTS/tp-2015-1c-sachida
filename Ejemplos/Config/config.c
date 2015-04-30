#include <stdlib.h>
#include <string.h>
#include <commons/config.h>
#include "config.h" //Aca linkeo el header que yo mismo arme
#include <stdio.h>

char *getCongifString(char *property, char *propertyDefault);
int getCongifInt(char *property, int propertyDefault);

t_configuracion* CONFIG; //t_configuracion Lo defino yo segun mi necesidad (ver .h)
static t_config* _config; //t_config es de las commons

void main(){ //Solo hago el main para poder probarlo y que muestre lo que quiero
	initConfig();
	printf("El valor de IP es %s\n", CONFIG->IP);
	printf("El valor de PUERTO es %d\n", CONFIG->PUERTO);
	printf("El valor de IP2 es %s\n", CONFIG->IP2);
	printf("El valor de PUERTO2 es %d\n", CONFIG->PUERTO2);
	freeConfig();
}

void initConfig(){

	//Primero tengo que armar el archivo de config a mano, voy a la carpeta "nuevo archivo" config.cfg
		_config = config_create("config.cfg"); //Le asigno a mi variable de tipo t_config el archivo de config que arme

	//A mi variable global (porque despues voy a usar la config en todos lados le asigno la memoria que necesita para guardar el los datos
	CONFIG = malloc(sizeof(t_configuracion));

	//Estas son todas iguales, consiguen un dato en base al config, todas reciben
	//Primer parametro, la key en el cfg
	//Segundo parametro, un default por si no habia una key
	CONFIG->IP = getCongifString("IP","127.0.0.1");
	CONFIG->PUERTO = getCongifInt("PUERTO",30000);
	CONFIG->IP2 = getCongifString("IP2","127.0.0.1");
	CONFIG->PUERTO2 = getCongifInt("PUERTO2",30000);

	//Destruyo_config para liberar memoria
	config_destroy(_config);
}

void freeConfig(){
	free(CONFIG);//Termine de usar la config, libero memoria
}

char *getCongifString(char *property, char *propertyDefault){
	if (config_has_property(_config, property)) //Me fijo si en el .cfg esta la property porque sino explota
		return strdup(config_get_string_value(_config, property)); //copio esa property
	return propertyDefault; //O sino mando el default
}

int getCongifInt(char *property, int propertyDefault){ //Idem anterior
	if (config_has_property(_config, property))
			return config_get_int_value(_config, property);
	return propertyDefault;
}

