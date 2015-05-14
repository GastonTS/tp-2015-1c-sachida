#include <commons/log.h>

void main(){

	t_log* logger; //t_log es el tipo que te dan las commons para usar los loger
	char *texto;

	logger = log_create("EjemploLog.log", "EJLOG", 1, log_level_from_string("TRACE"));
	//Primer Parametro, nombre del archivo log que voy a crear
	//Segundo Parametro, nombre con el que va a llamar al programa
	//Tercer Parametro 1- Muestra por pantalla  0- Solo graba en el log
	//Cuarto Parametro, Nivel de log	
		//TRACE, muestra TRACEs-DEBUGs-INFOs-WARNINGs-ERRORs
		//DEBUG, muestra DEBUGs-INFOs-WARNINGs-ERRORs
		//INFO, muestra INFOs-WARNINGs-ERRORs
		//WARNING, muestra WARNINGs-ERRORs
		//ERROR, muestra ERRORs

	//Estas funcionan todas igual, mando el t_logger al que quiero mandar un log
	//y como segundo parametro mando lo que quiero logear (como mandaria un printf	
	texto="Trace";
	log_trace(logger, "Esto es un %s", texto);

	texto="Debug";
	log_debug(logger, "Esto es un %s", texto);

	texto="Info";
	log_info(logger, "Esto es un %s", texto);

	texto="Warning";
	log_warning(logger, "Esto es un %s", texto);

	texto="Error";
	log_error(logger, "Esto es un %s", texto);
	
	//Destruyo para que no quede la memoria ocupada
	log_destroy(logger);
}
