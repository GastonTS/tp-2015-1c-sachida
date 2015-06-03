#include "Nodo.h"

void createNode(char *dirArchivo);
char* getBloque(int nroBloque);
void setBloque(int nroBloque);
void getFileContent();
void nodeMap(rutinaMap, int nroBloque);
void nodeReduce(int[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda);

//Le agregue los argumentos para que se pueda pasar el archivo de conf como parametro del main
int main(int argc, char *argv[]) {
	if(argc != 2)
		{
			printf("ERROR, la sintaxis del servidor es: ./Nodo.c archivo_configuracion \n");
			return -1;
		}
	//Creo el logger parametros -->
	//Nombre del archivo de log
	//nombre del programa que crea el log
	//se muestra el log por pantalla?
	//nivel minimo de log
	logger = log_create("Log.txt", "Node",1, log_level_from_string("DEBUG"));

	//Llamo a la funcion que esta abajo de todo que saca los datos del archivo de config
	getInfoConf(argv[1]);


	//createNode();
	/*pasar datos de los archivos de configuracion a constantes*/
	getBloque(5);
	return EXIT_SUCCESS;
}
/* Almacenar los datos del FS y hacer Map y Reduce segun lo requerido por los Jobs */
void createNode(char *dirArchivo) {

	//TODO DANI NO ENTIENDO QUE ES ESTE PARAMETRO

	 /*al crear el nodo con su respectivo bloque de datos, ponerle de nombre node_name
	Funcion de la biblioteca lisen. para esperar al FS
	 stat se consigue el tamaño de la rchivo
	truncate -s 1G miarchivo.bin*/

	/* VAN LOS SOCKETS*/
	//Primero se conecta al filesystem
	int sockfd, numbytes; //descriptores
	char* buffer;
	//Para enviar y recibir datos creamos un buffer o paquete donde se almacenan
	char buf[MAXDATASIZE];

	struct hostent *he;

	struct sockaddr_in fileSystem;

	t_mensaje mensaje;  //Estructura para intercambiar mensajes (PROTOCOLO)

	//Creo el buffer para comunicarme con el filesystem
	if((buffer = (char*) malloc (sizeof(char) * MAXDATASIZE)) == NULL)
	{
		log_error(logger, "Error al reservar memoria para el buffer en conectar con FileSystem");
		exit(-1);
	}

	if((sockfd = socket(AF_INET,SOCK_STREAM,0))==-1){
		printf("Error en crear el socket del FileSystem");
		exit(-1);
	}

	fileSystem.sin_family = AF_INET;
	fileSystem.sin_port = htons(puerto_fs);
	fileSystem.sin_addr.s_addr = inet_addr(ip_fs);
	memset(&(fileSystem.sin_zero),0,8);

	if(connect(sockfd,(struct sockaddr *)&fileSystem,sizeof(struct sockaddr))==-1){
		printf("error en el connect al FileSystem");
		exit(-1);
	}

	log_info(logger,"Se conecto al FileSystem correctamente");

	if((numbytes=recv(sockfd, buffer, SIZE_MSG,0))==-1){
		printf("Error en el recv handshake del fileSystem");
		exit(-1);
	}

	//Copio el mensaje que recibi en el buffer
	memcpy(&mensaje,buffer,SIZE_MSG);

	if((mensaje.id_proceso  == FILESYSTEM)&&(mensaje.tipo=HANDSHAKE))
	{
		log_info(logger, "Conexion Lograda con el FileSystem");
	}
	else
	{
		log_error(logger, "No recibi Handshake del FileSystem");
		exit(-1);
	}

	memset(buffer,'\0',MAXDATASIZE);

	mensaje.tipo = HANDSHAKEOK;
	mensaje.id_proceso = NODO;
	memcpy(buffer,&mensaje,SIZE_MSG);

	if((numbytes=send(sockfd,buffer,SIZE_MSG,0))<=0)
	{
		printf("Error en el send handshakeok al FileSystem");
		exit(-1);
	}

	//TODO SI ME PUDE CONECTAR AL FILESYSTEM ENTONCES CREAR EL ARCHIVO DE DATOS Y EL TMP
	//TODO ENGLOBAR LO DE ARRIBA EN UNA VARIABLE
	//TODO QUEDAR A LA ESPERA DEL FILESYSTEM, NODOS, O JOBS PARA REALIZAR DISTINTAS TAREAS


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
				log_error(logger, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size",file_name,size);
				//fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
				abort();
			}
			log_info("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);
			//printf ("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);

			//Se unmapea , y se cierrra el archivo
			munmap( mapeo, size );
			close(mapper);
			return mapeo;
		}


void setBloque(int nroBloque){}
/*Grabara los datos enviados*/

void getFileContent(){
/*Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 * Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
 * getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
 * por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
 *  es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.
 * */
}

int nodeMap (rutinaMap, int nroBloque){
	/* El Nodo4 guarda el contenido de map.py en un archivo en el filesystem local. Le da permisos de ejecución.
	 * El hilo mapper le solicita al Nodo4 que envie el contenido del Bloque6 por entrada estánda a map.py. El STDOUT lo almacena en un archivo temporal (ej: map.py.result.tmp)
	 * Usa la tool sort para ordenar el archivo temporal del paso anterior ya en el archivo definitivo
	# cat map.py.result.tmp | sort > librazo12347.tmp
	 * El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
	* los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper*/
	int lugarDeAlmacenamiento;
	return lugarDeAlmacenamiento;
}


void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
	//el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
	 /* El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
	 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
	 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino */
	return 0;
}



void getInfoConf(char* conf)
{
	t_config* config; //creamos la variable que va a ser el archivo de config

	config = config_create(conf); //creamos el "objeto" archivo de config

	strcpy(ip_fs,config_get_string_value(config,"IP_FS"));
	puerto_fs = config_get_int_value(config,"PUERTO_FS");
	strcpy(ip_nodo,config_get_string_value(config, "IP_NODO"));
	puerto_nodo = config_get_int_value(config, "PUERTO_NODO");
	strcpy(archivo_bin,config_get_string_value(config,"ARCHIVO_BIN"));
	strcpy(dir_tmp,config_get_string_value(config,"DIR_TMP"));
	strcpy(nodo_nuevo,config_get_string_value(config,"NODO_NUEVO"));

	puts("Extraccion correcta del archivo de configuracion");

	config_destroy(config); //destruimos el "objeto" archivo de config
}
/*
 * Obtiene todos los datos del archivo de configuracion y los guarda en variables
 * para que podamos utilizarlo a lo largo del programa
 * ACLARACION:
 * 			 El archivo de configuracion se pasa por parametro cuando se realiza la
 * 			 ejecucion: ./Nodo.c "rutaArchivoConfig"
 */

