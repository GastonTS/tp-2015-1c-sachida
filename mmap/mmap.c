#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

//Funcion que deveuelve el tamaño
int size_of(int fd){
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

int main (int argc, char *argv[]){
	int mapper;
	char* mapeo;
	int size;

	char* file_name = "./archivo_mmap.txt";
	//Se abre el archivo para solo lectura
	mapper = open (file_name, O_RDONLY);
	size = size_of(mapper);
	if( (mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper, 0 )) == MAP_FAILED){
		//Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
		fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
		abort();
	}

	printf ("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);

	//Se unmapea , y se cierrra el archivo
	munmap( mapeo, size );
	close(mapper);
	return EXIT_SUCCESS;
}
