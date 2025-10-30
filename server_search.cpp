#include "func_server.hpp"

int main() {
	// 1. Crear socket del servidor
	int serverFd = createNBSocket();
	if (serverFd < 0) {
		cerr << "[ERROR] No se pudo crear socket del servidor\n";
		return 1;
	}

	// 2. Crear instancia de epoll
	int epollFd = epoll_create1(0);
	if (epollFd < 0) {
		cerr << "[ERROR] No se pudo crear epoll\n";
		close(serverFd);
		unlink(SOCKET_PATH);
		return 1;
	}

	cout << "[SERVER] Epoll creado correctamente\n";

	// 3. Agregar socket del servidor al epoll
	if (addSocketToEpoll(epollFd, serverFd, EPOLLIN) < 0) {
		close(epollFd);
		close(serverFd);
		unlink(SOCKET_PATH);
		return 1;
	}

	cout << "[SERVER] Servidor listo, esperando conexiones...\n";

	// 4. Array de eventos
	struct epoll_event events[MAX_EVENTS];

	// 5. Bucle principal
	while (true) {
		// Esperar eventos (bloqueante hasta que llegue algo)
		int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1); // Number of file descriptors

		if (nfds < 0) {
			cerr << "[ERROR] Error en epoll_wait\n";
			break;
		}

		// Procesar todos los eventos
		for (int i = 0; i < nfds; i++) {
			int connectionFd = events[i].data.fd;
			uint32_t event = events[i].events;
			// Evento en el servidor = nueva conexión
			if (connectionFd == serverFd) {
				// Aceptar todas las conexiones pendientes
				while (true) {
					if (acceptNewClient(serverFd, epollFd) < 1) {
						break;
					}
				}
				continue;
			}

			if (event & EPOLLIN) {
				int messageReady = receiveFromClient(connectionFd, epollFd);
				if (messageReady == 1) { // Hay mensaje
					client_buffers[connectionFd] = searchServer(client_buffers[connectionFd]); // Procesamos el mensaje sobreescribiendo lo que había antes
					sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
				} else if (messageReady == -1) { // no hay mensaje
					client_buffers.erase(connectionFd); // borramos el mensaje en el map
				}
			}

			if (event & EPOLLOUT) {
				sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
			}
		}
	}

	// 6. Limpieza
	close(epollFd);
	close(serverFd);
	unlink(SOCKET_PATH);

	cout << "[SERVER] Servidor cerrado\n";
	return 0;
}