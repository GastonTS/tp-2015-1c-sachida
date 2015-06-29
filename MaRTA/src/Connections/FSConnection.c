#include "Connection.h"
#include "FSConnection.h"

void connectFS();

int fsSocket;

void initFSConnection() {
	fsSocket = -1;
	connectFS();
}

void FSConnectionLost() {
	log_error(logger, "Connection to FS was lost.");
	socket_close(fsSocket);
	fsSocket = -1;
}

void connectFS() {
	if (0 > fsSocket) {
		log_info(logger, "Connecting MDFS...");
		while (0 > fsSocket) {
			fsSocket = socket_connect(cfgMaRTA->fsIP, cfgMaRTA->fsPort);
		}
		if (fsSocket >= 0) {
			if (HANDSHAKE_FILESYSTEM != socket_handshake_to_server(fsSocket, HANDSHAKE_FILESYSTEM, HANDSHAKE_NODO)) {
				log_error(logger, "Handshake to filesystem failed.");
				FSConnectionLost();
			} else {
				log_info(logger, "Connected to FileSystem.");
			}
		}
	}
}
