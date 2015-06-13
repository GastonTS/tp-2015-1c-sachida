#ifndef SRC_SERIALIZE_SERIALIZE_H_
#define SRC_SERIALIZE_SERIALIZE_H_

#include "../structs/job.h"

t_job *desserealizeJob(int fd, uint32_t id);
void serialiceMapToOrder(int fd, t_map *map);

#endif
