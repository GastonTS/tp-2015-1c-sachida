#ifndef SRC_MARTA_H_
#define SRC_MARTA_H_

typedef struct {
	int puerto_listen;
	char *ip_fs;
	int puerto_fs;
} t_configMaRTA;

t_configMaRTA* cfgMaRTA;
t_log* logger;

int initConfig(char* archivoConfig);
void freeMaRTA();

#endif /* SRC_MARTA_H_ */
