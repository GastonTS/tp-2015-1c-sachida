#ifndef SRC_PLANNING_REDUCEPLANNING_H_
#define SRC_PLANNING_REDUCEPLANNING_H_

#include "../MaRTA.h"
#include "../structs/job.h"

void noCombinerReducePlanning(t_job *job);
void combinerPartialsReducePlanning(t_job *job);
void combinerFinalReducePlanning(t_job *job);

#endif
