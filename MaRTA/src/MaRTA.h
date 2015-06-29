#ifndef SRC_MARTA_H_
#define SRC_MARTA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/collections/list.h>

typedef struct {
	uint16_t listenPort;
	char *fsIP;
	uint16_t fsPort;
} t_configMaRTA;

extern t_configMaRTA *cfgMaRTA;
extern t_list *nodes;
extern t_log *logger;
extern uint16_t cantJobs;

#endif
