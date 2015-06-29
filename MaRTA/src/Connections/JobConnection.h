#ifndef SRC_CONNECTIONS_JOBCONNECTION_H_
#define SRC_CONNECTIONS_JOBCONNECTION_H_

#include "Connection.h"
#include "../structs/job.h"

void *acceptJob(void *params);
t_job *desserializeJob(int socket, uint16_t id);
void serializeMapToOrder(int socket, t_map *map);
void serializeReduceToOrder(int socket, t_reduce *reduce);
e_socket_status recvResult(int socket, t_job *job);
void desserializeMapResult(void *buffer, t_job *job);
void desserializaReduceResult(void *buffer, t_job *job);
void sendDieOrder(int socket);

#endif
