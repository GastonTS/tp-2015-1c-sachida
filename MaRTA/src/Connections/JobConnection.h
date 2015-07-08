#ifndef SRC_CONNECTIONS_JOBCONNECTION_H_
#define SRC_CONNECTIONS_JOBCONNECTION_H_

#include "Connection.h"
#include "../structs/job.h"

void *acceptJob(void *params);
t_job *desserializeJob(int socket, uint16_t id);
e_socket_status serializeMapToOrder(int socket, t_map *map);
e_socket_status serializeReduceToOrder(int socket, t_reduce *reduce);
char *recvResult(t_job *job);
void desserializeMapResult(void *buffer, t_job *job);
char *desserializaReduceResult(void *buffer, t_job *job);
e_socket_status sendDieOrder(int socket, uint8_t reason);

#endif
