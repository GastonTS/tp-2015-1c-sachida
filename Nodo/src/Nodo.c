#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

void createNode(char *dirArchivo);
char* getBloque(int nroBloque);
void setBloque(int nroBloque);
void getFileContent();
void nodeMap(rutinaMap, int nroBloque);
void nodeReduce(int[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda);

int main(void) {
	createNode();
	/*pasar datos de los archivos de configuracion a constantes*/
	getBloque(5);
	return EXIT_SUCCESS;
}
/* Almacenar los datos del FS y hacer Map y Reduce segun lo requerido por los Jobs */
void createNode(char *dirArchivo) {
	 /*al crear el nodo con su respectivo bloque de datos, ponerle de nombre node_name
	Funcion de la biblioteca lisen. para esperar al FS
	 stat se consigue el tamaño de la rchivo
	truncate -s 1G miarchivo.bin*/
}

int size_of(int fd){
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

char* getBloque(int nroBloque){
			int mapper;
			char* mapeo;
			int size;
			int pagesize;
			const sizemapper;
			char* file_name = "/home/utnso/Sachida/tp-2015-1c-sachida/Ejemplos/Mmap/src/archivo_mmap.txt"; //Aca tiene que abrir el archivo que crea en el createNodo
			//Se abre el archivo para solo lectura
			mapper = open (file_name, O_RDONLY);
			pagesize = getpagesize();
			size = size_of(mapper);
			//size = 20;
			//Trate size bytes a partir de la posicion pagesize*(nroBloque-1)
			if( (mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper, pagesize*(nroBloque-1) )) == MAP_FAILED){
				//Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
				fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
				abort();
			}

			printf ("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);

			//Se unmapea , y se cierrra el archivo
			munmap( mapeo, size );
			close(mapper);
			return mapeo
		}



}

void setBloque(int nroBloque){}
/*Grabara los datos enviados*/

void getFileContent(){}
/*Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 * Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal*/

void nodeMap (rutinaMap, int nroBloque){
	return lugarDeAlmacenamiento;
}

/* El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
* los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper
*/

void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
	//el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
	return 0;
}
 /* El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino */

