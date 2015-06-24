/*
 * marta.h
 *
 *  Created on: 23/6/2015
 *      Author: utnso
 */

#ifndef SRC_MARTA_MARTA_H_
#define SRC_MARTA_MARTA_H_


/************* METODOS *************/

int conectarMarta();
void atenderMarta(int socketMarta);
void atenderMapper(void* parametros);
void confirmarMap(char confirmacion, t_map* map);
void atenderReducer(void* parametros);
void confirmarReduce(char confirmacion, t_reduce* reduce, void* bufferNodo);

/*serialize*/
void serializeConfigMaRTA(int fd, bool combiner, char* files);
void recvOrder(int fd);
t_map* desserializeMapOrder(void *buffer);
void desserializeTempToList(t_list *temporals, void *buffer, size_t *sbuffer);
t_reduce* desserializeReduceOrder(void *buffer, size_t sbuffer);

#endif /* SRC_MARTA_MARTA_H_ */
