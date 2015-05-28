#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

void createNodo(char *dirArchivo);
void getBloque(int nroBloque);
void setBloque(int nroBloque);
void getFileContent();
void nodoMap(rutinaMap, int nroBloque);
void nodoReduce(int[string nombreNodo, int nroBloque], rutinaReduce, char nombreDondeGuarda);

int main(void) {
	createNodo();
	return EXIT_SUCCESS;
}
/* Almacenar los datos del FS y hacer Map y Reduce segun lo requerido por los Jobs */
void createNodo(char *dirArchivo) {
	//Funcion de la biblioteca lisen. para esperar al FS
	// stat se consigue el tamaño de la rchivo
	//truncate -s 1G miarchivo.bin
}

void getBloque(int intBloque){
	int mapper, size;
	char* mapeo;
	size = size_of(mapper);
	mapper = open (file_name, O_RDONLY);
	mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper, 0 );
}
/*Devolvera el contenido del bloque solicitado*/
void setBloque(int nroBloque){}
/*Grabara los datos enviados*/

void getFileContent(){}
/*Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 * Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal*/

void nodoMap (rutinaMap, int nroBloque){
	return lugarDeAlmacenamiento;
}

/* El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
* los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper
*/

void nodoReduce (array[string nombreNodo, int nroBloque], rutinaReduce, char nombreDondeGuarda){
	//el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
	return 0;
}
 /* El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino */
