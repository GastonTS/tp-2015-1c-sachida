#ifndef CONNECTIONS_MARTA_H
#define CONNECTIONS_MARTA_H

#define MARTA_COMMAND_GET_FILE_BLOCKS 1

void connections_marta_initialize();
void connections_marta_shutdown();

void connections_marta_accept(int socketAccepted);

#endif
