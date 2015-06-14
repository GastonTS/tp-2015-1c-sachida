#ifndef SRC_MARTA_H_
#define SRC_MARTA_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdint.h>

extern t_list *nodes;
extern t_log *logger;
extern int fdListener;
extern int fdAccepted;
extern bool exitMaRTA;
extern uint16_t cantJobs;

#endif
