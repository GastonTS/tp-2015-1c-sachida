#ifndef SRC_SERIALIZE_SERIALIZE_H_
#define SRC_SERIALIZE_SERIALIZE_H_

#include "../structs/job.h"

t_job *desserealizeJob(int fd, uint16_t id);
void serializeMapToOrder(int fd, t_map *map);
void serializeReduceToOrder(int fd, t_reduce *reduce);

#endif
