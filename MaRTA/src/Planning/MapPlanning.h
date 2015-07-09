#ifndef SRC_PLANNING_MAPPLANNING_H_
#define SRC_PLANNING_MAPPLANNING_H_

#include "../MaRTA.h"
#include "../structs/job.h"

void planMaps(t_job *job);
int rePlanMap(t_job *job, t_map *map);
void notifFileUnavailable(t_job *job);

#endif
