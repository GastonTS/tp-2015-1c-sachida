#include "Nodo.h"
#include "utils/socket.h"
#include "connections/connections.h"


t_nodeCfg *nodeCfg;

void createNode();
//void getFileContent();
//void nodeMap(rutinaMap, int nroBloque);
//void nodeReduce(int[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda);
size_t size_of(int fd);
size_t fileSize;
int conectarJob(t_nodeCfg *config);
int socket_job;
void freeNodo();

t_log* node_logger;

//Le agregue los argumentos para que se pueda pasar el archivo de conf como parametro del main
int main(int argc, char *argv[]) {
	node_logger = log_create("node.log", "Node", 1, log_level_from_string("TRACE"));

	if (argc != 2) {
		log_error(node_logger, "Missing config file");
		freeNodo();
		return EXIT_FAILURE;
	}
	if (!initConfig(argv[1])) {
		log_error(node_logger, "Config failed");
		freeNodo();
		return EXIT_FAILURE;
	}

	uint16_t cantBloques = 30; // Le voy a decir que tengo 10 bloques para usar.

	//if (nodeCfg->nodo_nuevo) {
	if (1) {
		int fd = open("/home/utnso/block", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd == -1) {
			return -1; // TODO handlear.
		}

		lseek(fd, BLOCK_SIZE * cantBloques, SEEK_SET);
		write(fd, "", 1);
		lseek(fd, 0, SEEK_SET);

		close(fd);
	}

	connections_initialize(nodeCfg);
	while(1) {} // TODO
	//socket_job = conectarJob();

	//nodo_escucharAcciones(socket_job);
	freeNodo();
	return EXIT_SUCCESS;
}

// Almacenar los datos del FS y hacer Map y Reduce segun lo requerido por los Jobs
void createNode() {
	//TODO DANI NO ENTIENDO QUE ES ESTE PARAMETRO
	//fileSize = size_of(archivo_bin);
	//printf("%d\n", fileSize);
	/*Funcion de la biblioteca lisen. para esperar al FS
	 stat se consigue el tamaño de la rchivo
	 truncate -s 1G miarchivo.bin*/

	//VAN LOS SOCKETS
	//Primero se conecta al filesystem
	//TODO SI ME PUDE CONECTAR AL FILESYSTEM ENTONCES CREAR EL ARCHIVO DE DATOS Y EL TMP
//tamanioArchivoDatos = size_of(archivo_bin);
	//TODO ENGLOBAR LO DE ARRIBA EN UNA VARIABLE
	//TODO QUEDAR A LA ESPERA DEL FILESYSTEM, NODOS, O JOBS PARA REALIZAR DISTINTAS TAREAS
}

/*void escucharJobs(){

 fd_set read_fds;
 fd_set master;
 int fdmax;
 int i;
 int sock_escucha, new_socket;


 sock_escucha = escuchar();

 if(sock_escucha == -1 )
 {
 printf("ERROR, error iniciando sockets. \n");
 printf("ERROR, sock_escucha = %d \n", sock_escucha);
 exit(-1) ;
 }

 while(exit==1)
 {
 read_fds = master;
 if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
 {
 log_error(logger, "Error en la funcion select de escucharJobs");
 exit(-1);
 }
 for (i = 0; i <= fdmax; i++)
 {
 //pregunto si en algun socket hubo algun cambio
 if (FD_ISSET(i, &read_fds))
 {
 if (i == sock_escucha)
 {
 //si es la consola es porque el listen recibio una nueva conexion
 new_socket = escuchar_puerto(sock_escucha);
 if(new_socket == -1)
 {
 //si es -1 fallo asi que continuo descartando esta
 log_error(logger,"No se pudo agregar una nueva conexion");
 continue;
 }
 //lo agrego para que lo escuchen en el select
 FD_SET(new_socket, &master);
 //si es el mayor lo re asigno
 if(fdmax < new_socket)
 fdmax = new_socket;

 continue;
 }

 if(EstaConectadaCPU(i) != 0)
 {
 //si es distinto de 0 la cpu me esta hablando
 log_debug(logger,"Se detecto actividad en un CPU");

 //escucho la cpu que me habla
 if(atender_cpu(i)==-1)
 {
 log_debug(logger,"Se cayo una CPU");
 close(i);
 FD_CLR(i, &master);
 //hago un metodo que saque la cpu de la lista de cpus
 cpu_remove(i);
 //TODO bajo el semaforo ya que hay una cpu menos
 //sem_wait(&sem_cpu_list);
 continue;
 }
 continue;
 }
 //si i no pertenece a ninguno lo saco
 //close(i);
 //FD_CLR(i, &master);
 }
 }
 }
 log_destroy(logger);
 return;
 }
 */

/*void escucharNodos(){}*/

/*int escuchar()
 {
 //llega a hacer el listen
 int sock_escucha;
 int yes=1;
 struct sockaddr_in my_addr;

 if( (sock_escucha=socket(AF_INET,SOCK_STREAM,0))==-1)
 {
 log_error(logger, "Error en funcion socket en escuchar");
 return -1;
 }
 //este nose si va
 if(setsockopt(sock_escucha,SOL_SOCKET,SO_REUSEADDR, &yes,sizeof(int))==-1)
 {
 log_error(logger, "Error en funcion setsockopt en escuchar");
 return -1;

 }
 my_addr.sin_port=htons(puerto_nodo);
 my_addr.sin_family=AF_INET;
 my_addr.sin_addr.s_addr=ip_nodo;
 memset(&(my_addr.sin_zero),0,8);

 if (bind(sock_escucha,(struct sockaddr *)&my_addr,sizeof(struct sockaddr))==-1)
 {
 log_error(logger, "Error en funcion bind en escuchar");
 return -1;
 }

 if (listen(sock_escucha,BACKLOG)==-1)
 {
 log_error(logger, "Error en funcion listen en escuchar");
 return -1;
 }

 return sock_escucha;
 }*/

// TODO el job se conecta al nodo mepa eh... hay que poner un listening.
int conectarJob(t_nodeCfg *config) {
		int handshakea;
		char* jobIp;
		int nodoSocket = socket_listen(config->puerto_nodo);
		int jobSocket = socket_accept_and_get_ip(nodoSocket, &jobIp);
		free(jobIp);
		handshakea = socket_handshake_to_client(jobSocket,
		HANDSHAKE_NODO, HANDSHAKE_JOB);
		printf("derror %d", handshakea);
		log_info(node_logger, "Conection sucessfully");
		return jobSocket;
}

char* node_getBlock(uint16_t numBlock) {
	log_info(node_logger, "Getting block number %d", numBlock);

	int fd = open("/home/utnso/block", O_RDONLY); // todo name.
	if (fd == -1) {
		log_error(node_logger, "An error occurred while trying to open the bin file.");
		return NULL;
	}

	char *blockStr = (char *) mmap(0, BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, numBlock * BLOCK_SIZE);

	close(fd);

	return blockStr;
}

void node_freeBlock(char *blockStr) {
	munmap(blockStr, BLOCK_SIZE);
}
void node_setBlock(uint16_t numBlock, char *blockData) {
	log_info(node_logger, "Setting block number %d", numBlock);

	int fd = open("/home/utnso/block", O_RDWR); // TODO name.
	if (fd == -1) {
		log_error(node_logger, "An error occurred while trying to open the bin file.");
		return;
	}

	char *fileBlockStr = (char *) mmap(0, BLOCK_SIZE, PROT_WRITE, MAP_SHARED, fd, numBlock * BLOCK_SIZE);

	memcpy(fileBlockStr, blockData, strlen(blockData) + 1);

	munmap(fileBlockStr, BLOCK_SIZE);
	close(fd);
}

/*void getFileContent(){
 Devolverá   el   contenido   del   archivo   de   Espacio   Temporal solicitado.
 Se usara en el return de las funciones para devolver los archivos almencenadaso en memoria temporal
 getFileContent probablemente no sea tan "útil" como usuario, pero sí la usan los Nodos para pasarse datos para el Reduce, y, ya que está, exponérsela al FS ayuda a que,
 por ejemplo, mientras desarrollan puedan chequear de manera "fácil" que los temporales se estén generando bien. Poder inspeccionar lo que está pasando en el sistema siempre
 es bueno, y si encima viene casi gratis en cuanto a esfuerzo de desarrollo, mejor.

 }*/

/*int nodeMap (rutinaMap, int nroBloque){
 El Nodo4 guarda el contenido de map.py en un archivo en el filesystem local. Le da permisos de ejecución.
 * El hilo mapper le solicita al Nodo4 que envie el contenido del Bloque6 por entrada estánda a map.py. El STDOUT lo almacena en un archivo temporal (ej: map.py.result.tmp)
 * Usa la tool sort para ordenar el archivo temporal del paso anterior ya en el archivo definitivo
 # cat map.py.result.tmp | sort > librazo12347.tmp
 * El hilo mapper se conecta al nodo, y le indica la rutina de maping, el bloque de datos donde aplicarla y tiene que almacenar
 * los resultados de manera ordenada (sort) en el FS Temporal del nodo. Debera dar una respuesta al hilo MApper
 int lugarDeAlmacenamiento;
 return lugarDeAlmacenamiento;
 }*/

/*void nodeReduce (array[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda){
 //el reduce recibe un nodo y un nombre de archivo (el FS se encargara de rearmar ese archivo y pasarlo)
 El hilo reduce, indica aplicar la rutina sobre varios archvos del espacio temporal, de los cuales uno debe ser siempre local al nodo
 * El reduce le manda el nombre de los bloques y los nodos donde se encuentran, el codigo de la rutina de reduce y el nombre del
 * archivo donde se alamcenara. Al finalizar se debe informar al JOB que termino
 return 0;
 }*/

int initConfig(char* configFile) {
	t_config* _config;
	int failure = 0;

	int getConfigInt(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_int_value(_config, property);
		}

		failure = 1;
		log_error(node_logger, "Config not found for key %s", property);
		return -1;
	}

	char* getCongifString(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(node_logger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(configFile);
	nodeCfg = malloc(sizeof(t_nodeCfg));
	log_info(node_logger, "Loading config...");

	nodeCfg->archivo_bin = strdup(getCongifString("ARCHIVO_BIN"));
	nodeCfg->dir_tmp = strdup(getCongifString("DIR_TMP"));
	nodeCfg->ip_fs = strdup(getCongifString("IP_FS"));
	nodeCfg->ip_nodo = strdup(getCongifString("IP_NODO"));
	nodeCfg->ip_job = strdup(getCongifString("IP_JOB"));
	nodeCfg->nodo_nuevo = getConfigInt("NODO_NUEVO");
	nodeCfg->puerto_fs = getConfigInt("PUERTO_FS");
	nodeCfg->puerto_nodo = getConfigInt("PUERTO_NODO");
	nodeCfg->puerto_job = getConfigInt("PUERTO_JOB");

	if (!failure) {
		log_info(node_logger, "Archivo bin: %s", nodeCfg->archivo_bin);
		log_info(node_logger, "Dir temporal: %s", nodeCfg->dir_tmp);
		log_info(node_logger, "FileSystem IP: %s", nodeCfg->ip_fs);
		log_info(node_logger, "FileSystem Port: %d", nodeCfg->puerto_fs);
		log_info(node_logger, "Job IP: %s", nodeCfg->ip_job);
		log_info(node_logger, "Job Port: %d", nodeCfg->puerto_job);
		log_info(node_logger, "Node IP: %s", nodeCfg->ip_nodo);
		log_info(node_logger, "Node Port: %d", nodeCfg->puerto_nodo);
		log_info(node_logger, "New Node: %d", nodeCfg->nodo_nuevo);
	}

	config_destroy(_config);
	return !failure;
}

/*
 * Obtiene todos los datos del archivo de configuracion y los guarda en variables
 * para que podamos utilizarlo a lo largo del programa
 * ACLARACION:
 * 			 El archivo de configuracion se pasa por parametro cuando se realiza la
 * 			 ejecucion: ./Nodo.c "rutaArchivoConfig"
 */

void freeNodo() {
	if (nodeCfg->archivo_bin) {
		free(nodeCfg->archivo_bin);
	}
	if (nodeCfg->dir_tmp) {
		free(nodeCfg->dir_tmp);
	}
	if (nodeCfg->ip_fs) {
		free(nodeCfg->ip_fs);
	}
	if (nodeCfg->ip_nodo) {
		free(nodeCfg->ip_nodo);
	}
	if (nodeCfg->ip_job) {
		free(nodeCfg->ip_job);
	}
	free(nodeCfg);
	log_destroy(node_logger);
}

size_t size_of(int fd) {
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

