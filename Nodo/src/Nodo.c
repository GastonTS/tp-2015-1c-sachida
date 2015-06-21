#include "Nodo.h"
#include "socket.h"

typedef struct {
	uint16_t puerto_fs;
	uint16_t puerto_job;
	uint16_t puerto_nodo;
	char *ip_nodo;
	char *ip_fs;
	char *ip_job;
	char *archivo_bin;
	char *dir_tmp;
	uint8_t nodo_nuevo;
} t_configNodo;

t_configNodo *cfgNodo;

void createNode();
//void getFileContent();
//void nodeMap(rutinaMap, int nroBloque);
//void nodeReduce(int[string nameNode, int nroBloque], rutinaReduce, char nombreDondeGuarda);
size_t size_of(int fd);
size_t fileSize;
int conectarFileSystem();
int conectarJob();
int socket_fileSystem;
int socket_job;
uint8_t obtenerComando(char* paquete);
uint16_t obtenerNumBlock(char* paquete);
uint32_t obtenerSize(char* paquete);
char* obtenerDatosBloque(char* paquete, uint32_t size);
void freeNodo();
void nodo_escucharAcciones(int socket);
void deserializeSetBlock(void *paquete);
void deserializeGetBlock(void *paquete, int fsSocket);

//Le agregue los argumentos para que se pueda pasar el archivo de conf como parametro del main
int main(int argc, char *argv[]) {
	nodeLogger = log_create("Nodo.log", "Nodo", 1, log_level_from_string("TRACE"));

	if (argc != 2) {
		log_error(nodeLogger, "Missing config file");
		freeNodo();
		return EXIT_FAILURE;
	}
	if (!initConfig(argv[1])) {
		log_error(nodeLogger, "Config failed");
		freeNodo();
		return EXIT_FAILURE;
	}

	uint16_t cantBloques = 30; // Le voy a decir que tengo 10 bloques para usar.

	//if (cfgNodo->nodo_nuevo) {
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

	socket_fileSystem = conectarFileSystem();
	//socket_job = conectarJob();

	// TODO, esto no vendria de config?

	char myName[] = "Nodo1"; // Le paso mi nombre.

	uint16_t sName = strlen(myName);
	size_t sBuffer = sizeof(cfgNodo->nodo_nuevo) + sizeof(cantBloques) + sizeof(sName) + sName;

	uint16_t cantBloquesSerialized = htons(cantBloques);
	uint16_t sNameSerialized = htons(sName);

	void *pBuffer = malloc(sBuffer);
	memcpy(pBuffer, &cfgNodo->nodo_nuevo, sizeof(cfgNodo->nodo_nuevo));
	memcpy(pBuffer + sizeof(cfgNodo->nodo_nuevo), &cantBloquesSerialized, sizeof(cantBloques));
	memcpy(pBuffer + sizeof(cfgNodo->nodo_nuevo) + sizeof(cantBloques), &sNameSerialized, sizeof(sName));
	memcpy(pBuffer + sizeof(cfgNodo->nodo_nuevo) + sizeof(cantBloques) + sizeof(sName), &myName, sName);

	socket_send_packet(socket_fileSystem, pBuffer, sBuffer);
	//socket_send_packet(socket_job, pBuffer, sBuffer);
	free(pBuffer);

	nodo_escucharAcciones(socket_fileSystem);
	//nodo_escucharAcciones(socket_job);
	freeNodo();
	return EXIT_SUCCESS;
}

void nodo_escucharAcciones(int socket) {
	while (1) {
		size_t packet_size;
		void* paquete;
		// Ahora armo todo para esperar a que el fs me mande datos.
		printf("armo todo para esperar a que el cliente mande datos: \n");
		e_socket_status status = socket_recv_packet(socket, &paquete, &packet_size);
		if (0 > status) {
			// TODO HANDLE
			printf("ERROR\n");
			fflush(stdout);
			return;
		}
		printf("Recive OK\n");
		uint8_t comando = obtenerComando(paquete);
		switch (comando) {
		case 1: //setBloque
			deserializeSetBlock(paquete);
			break;
		case 2: //getBloque
			deserializeGetBlock(paquete, socket);
			break;
		default:
			log_error(nodeLogger, "No es un comando válido");
			break;
			//default =  log_error("Log.txt", "Node",1,log_level_from_string("ERROR"));
			//TODO ACA DEBERIAMOS HACER EL WHILE INFINITO ESPERANDO CONEXIONES Y PETICIONES
			//TODO HAY QUE ABRIR UN THREAD PARA ESCUCHAR JOBS Y UNO PARA ESCUCHAR NODOS(Paralelismo)
			//ptrhead_create(&conexionesJob,NULL,(void*)escucharJobs,NULL);
			//pthread_create(&conexionesNodo,NULL,(void*)escucharNodos,NULL);
			// TODO TODAS LAS FUNCIONES GETBLOQUE Y ESAS VAN ADENTRO DE LOS TRHEADS
		}
		free(paquete);
	}
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

int conectarFileSystem() {
	int descriptorFileSystem = -1;
	while (0 > descriptorFileSystem) {
		descriptorFileSystem = socket_connect(cfgNodo->ip_fs, cfgNodo->puerto_fs);
	}
	if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(descriptorFileSystem, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
		return -1;
	}
	log_info(nodeLogger, "Conection sucessfully");
	return descriptorFileSystem;
}

// TODO el job se conecta al nodo mepa eh... hay que poner un listening.
int conectarJob() {
	int descriptorJob;
	int handshakea;
	descriptorJob = socket_connect(cfgNodo->ip_job, cfgNodo->puerto_job);
	handshakea = socket_handshake_to_server(descriptorJob,
	HANDSHAKE_JOB, HANDSHAKE_NODO);
	printf("derror %d", handshakea);
	log_info(nodeLogger, "Conection sucessfully");
	return descriptorJob;
}

void sendBloque(uint16_t nroBloque, int fsSocket) {
	printf("llego al sendBloque\n");
	printf("numero de bloque : %d\n", nroBloque);

	int fd = open("/home/utnso/block", O_RDONLY);
	if (fd == -1) {
		printf("ERROR OPEN\n");
		return; // TODO handlear.
	}

	char *blockStr = (char *) mmap(0, BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, nroBloque * BLOCK_SIZE);
	//blockStr[BLOCK_SIZE] = '\0';
	printf("strlen %d\n", strlen(blockStr));

	e_socket_status status = socket_send_packet(fsSocket, blockStr, strlen(blockStr));

	printf("STATUS %d\n", status);
	if (status != SOCKET_ERROR_NONE) {
		// TODO, manejar el error.
	}

	munmap(blockStr, BLOCK_SIZE);
	close(fd);

	/*int mapper;
	 char* mapeo;
	 size_t size;
	 size_t pagesize;
	 //Se abre el archivo para solo lectura

	 mapper = open(cfgNodo->archivo_bin, O_RDONLY);
	 pagesize = getpagesize();
	 size = size_of(mapper);
	 //size = 20;
	 //Trate size bytes a partir de la posicion pagesize*(nroBloque-1)
	 if ((mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper,
	 pagesize * (nroBloque - 1))) == MAP_FAILED) {
	 //Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
	 log_error(logger,
	 "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size",
	 cfgNodo->archivo_bin, size);
	 //fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
	 abort();
	 }
	 log_info(logger, "Tamaño del archivo: %d\n", size);
	 //printf ("Tamaño del archivo: %d\nContenido:'%s'\n", size, mapeo);

	 //Se unmapea , y se cierrra el archivo
	 munmap(mapeo, size);
	 close(mapper);
	 return mapeo;*/
}

void setBloque(uint16_t nroBloque, char* string) {
	printf("llego al setBLoque\n");
	printf("numero de bloque : %d\n", nroBloque);

	int fd = open("/home/utnso/block", O_RDWR);
	if (fd == -1) {
		printf("ERROR OPEN\n");
		return; // TODO handlear.
	}

	char *blockStr = (char *) mmap(0, BLOCK_SIZE, PROT_WRITE, MAP_SHARED, fd, nroBloque * BLOCK_SIZE);

	memcpy(blockStr, string, strlen(string) + 1);

	munmap(blockStr, BLOCK_SIZE);
	close(fd);

	/*
	 int mapper;
	 char* mapeo;
	 int size;
	 int pagesize;
	 //Se abre el archivo para lectura y escritura

	 mapper = open(cfgNodo->archivo_bin, O_WRONLY);
	 pagesize = getpagesize();
	 //Size debe llegar a 20mb asi los bloques son de 20mb
	 size = size_of(mapper);
	 //Trate size bytes a partir de la posicion pagesize*(nroBloque-1)
	 if ((mapeo = mmap( NULL, size, PROT_READ, MAP_SHARED, mapper,
	 pagesize * (nroBloque - 1))) == MAP_FAILED) {
	 //Si no se pudo ejecutar el MMAP, imprimir el error y abortar;
	 log_error(logger,
	 "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size",
	 cfgNodo->archivo_bin, size);
	 //fprintf(stderr, "Error al ejecutar MMAP del archivo '%s' de tamaño: %d: %s\nfile_size", file_name, size, strerror(errno));
	 abort();
	 }
	 //Aca se tiene que mandar lo que tiene string adentro de mapeo.
	 // TODO . fputs(pagesize * (nroBloque - 1), mapeo);
	 //Se unmapea , y se cierrra el archivo
	 munmap(mapeo, size);
	 close(mapper);

	 //Recibe un buffer de datos,despues con el puntero que me devuelve el mmap modifico el archivo mapeado, primero busco puntero[ j ]=\0 y lo saco,
	 // relleno los espacios que falten hasta el nuevo bloque y remplazo el puntero[ j ]=datos[a] ,agrego el \0 y cierro el mmap.
	 */
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
		log_error(nodeLogger, "Config not found for key %s", property);
		return -1;
	}

	char* getCongifString(char *property) {
		if (config_has_property(_config, property)) {
			return config_get_string_value(_config, property);
		}

		failure = 1;
		log_error(nodeLogger, "Config not found for key %s", property);
		return "";
	}

	_config = config_create(configFile);
	cfgNodo = malloc(sizeof(t_configNodo));
	log_info(nodeLogger, "Loading config...");

	cfgNodo->archivo_bin = strdup(getCongifString("ARCHIVO_BIN"));
	cfgNodo->dir_tmp = strdup(getCongifString("DIR_TMP"));
	cfgNodo->ip_fs = strdup(getCongifString("IP_FS"));
	cfgNodo->ip_nodo = strdup(getCongifString("IP_NODO"));
	cfgNodo->ip_job = strdup(getCongifString("IP_JOB"));
	cfgNodo->nodo_nuevo = getConfigInt("NODO_NUEVO");
	cfgNodo->puerto_fs = getConfigInt("PUERTO_FS");
	cfgNodo->puerto_nodo = getConfigInt("PUERTO_NODO");
	cfgNodo->puerto_job = getConfigInt("PUERTO_JOB");

	if (!failure) {
		log_info(nodeLogger, "Archivo bin: %s", cfgNodo->archivo_bin);
		log_info(nodeLogger, "Dir temporal: %s", cfgNodo->dir_tmp);
		log_info(nodeLogger, "FileSystem IP: %s", cfgNodo->ip_fs);
		log_info(nodeLogger, "FileSystem Port: %d", cfgNodo->puerto_fs);
		log_info(nodeLogger, "Job IP: %s", cfgNodo->ip_job);
		log_info(nodeLogger, "Job Port: %d", cfgNodo->puerto_job);
		log_info(nodeLogger, "Node IP: %s", cfgNodo->ip_nodo);
		log_info(nodeLogger, "Node Port: %d", cfgNodo->puerto_nodo);
		log_info(nodeLogger, "New Node: %d", cfgNodo->nodo_nuevo);
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
	if (cfgNodo->archivo_bin) {
		free(cfgNodo->archivo_bin);
	}
	if (cfgNodo->dir_tmp) {
		free(cfgNodo->dir_tmp);
	}
	if (cfgNodo->ip_fs) {
		free(cfgNodo->ip_fs);
	}
	if (cfgNodo->ip_nodo) {
		free(cfgNodo->ip_nodo);
	}
	if (cfgNodo->ip_job) {
		free(cfgNodo->ip_job);
	}
	free(cfgNodo);
	log_destroy(nodeLogger);
}

size_t size_of(int fd) {
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

uint8_t obtenerComando(char* paquete) {
	uint8_t command;
	memcpy(&command, paquete, sizeof(uint8_t));
	return command;
}

uint16_t obtenerNumBlock(char* paquete) {
	uint16_t numBlock;
	memcpy(&numBlock, paquete + sizeof(uint8_t), sizeof(uint16_t));
	numBlock = htons(numBlock);
	return numBlock;
}

uint32_t obtenerSize(char* paquete) {
	uint32_t size;
	memcpy(&size, paquete + sizeof(uint8_t) + sizeof(uint16_t), sizeof(uint32_t));
	size = ntohl(size);
	return size;
}

char* obtenerDatosBloque(char* paquete, uint32_t size) {
	char* packet = malloc(sizeof(char) * (size + 1));
	memcpy(packet, paquete + sizeof(uint8_t) + sizeof(uint16_t) + sizeof(uint32_t), size);
	packet[size] = '\0';
	return packet;
}

void deserializeSetBlock(void *paquete) {
	uint16_t numBlock;
	uint32_t pack_size;
	char * datosBloque;
	numBlock = obtenerNumBlock(paquete);
	pack_size = obtenerSize(paquete);
	datosBloque = obtenerDatosBloque(paquete, pack_size);
	setBloque(numBlock, datosBloque);
	free(datosBloque);
}

void deserializeGetBlock(void *paquete, int fsSocket) {
	uint16_t numBlock;
	numBlock = obtenerNumBlock(paquete);
	sendBloque(numBlock, fsSocket);
}
