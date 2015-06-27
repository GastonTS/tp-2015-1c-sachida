#ifndef SRC_SERIALIZE_SERIALIZE_H_
#define SRC_SERIALIZE_SERIALIZE_H_

#include "../structs/job.h"
#include "../../../utils/socket.h"

t_job *desserealizeJob(int fd, uint16_t id);
void serializeMapToOrder(int fd, t_map *map);
void serializeReduceToOrder(int fd, t_reduce *reduce);
e_socket_status recvResult(int fd, t_job *job);
void desserializeMapResult(void *buffer, t_job *job);
void desserializaReduceResult(void *buffer, t_job *job);
void sendDieOrder(int fd);

#endif
