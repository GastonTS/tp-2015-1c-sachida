#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <commons/config.h>
#include <commons/collections/list.h>
#include "../src/structs/node.h"
#include "../src/structs/job.h"
#include "../src/Serialize/serialize.h"
#include "../src/MaRTA.h"
#include "../../utils/socket.h"

void seializeCompleteJobTest() {
	while (!exitMaRTA) {
		printf("Waiting conecttion...\n");
		fflush(stdout);
		fdAccepted = socket_accept(fdListener);
		switch (socket_handshake_to_client(fdAccepted, HANDSHAKE_MARTA, HANDSHAKE_FILESYSTEM | HANDSHAKE_JOB)) {
		case HANDSHAKE_FILESYSTEM:
			printf("\nEl FileSystem!\n");
			break;
		case HANDSHAKE_JOB:
			cantJobs++;
			//TODO levantar un hilo
			t_job *job = desserealizeJob(fdAccepted, cantJobs);

			if (job->combiner)
				log_info(logger, "Iniciando Job: %d (Combiner)", job->id);
			else
				log_info(logger, "Iniciando Job: %d (No combiner)", job->id);

			t_map *map = CreateMap(1, 13, 30123, "NodoX", "xxx.yyy.zzz.www", "temporal map 1.txt");
			list_add(job->maps, map);

			serializeMapToOrder(fdAccepted, map);
			printf("\nMap done:%d", map->done);
			recvResult(fdAccepted, job);
			printf("Map done:%d\n\n", map->done);

			job->finalReduce->done = 0;
			job->finalReduce->finalNode = strdup("Nodo Final");
			job->finalReduce->nodeIP = strdup("x.y.z.w");
			job->finalReduce->nodePort = 54321;
			strcpy(job->finalReduce->tempResultName, "wasabisarasajuanatenaten");
			t_temp *temp1 = CreateTemp("ip del temporal 1", 12345, 15, "JA! mas mas y mas");
			list_add(job->finalReduce->temps, temp1);
			serializeReduceToOrder(fdAccepted, job->finalReduce);
			printf("\nReduce done:%d", job->finalReduce->done);
			recvResult(fdAccepted, job);
			printf("Reduce done:%d\n\n", job->finalReduce->done);
			sendDieOrder(fdAccepted);
			freeJob(job);

			if (cantJobs == 3) //XXX
				exitMaRTA = 1;
			break;
		}
	}
	fflush(stdout);
}
