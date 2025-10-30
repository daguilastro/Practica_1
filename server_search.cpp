#include "func_server.hpp"

// Variable global para controlar el cierre del servidor
bool server_running = true;

int main() {
	// Crear socket del servidor
	int serverFd = createNBSocket();
	if (serverFd < 0) {
		cerr << "[ERROR] No se pudo crear socket del servidor\n";
		return 1;
	}

	// Crear instancia de epoll
	int epollFd = epoll_create1(0);
	if (epollFd < 0) {
		cerr << "[ERROR] No se pudo crear epoll\n";
		close(serverFd);
		unlink(SOCKET_PATH);
		return 1;
	}

	cout << "[SERVER] Epoll creado correctamente\n";

	// Agregar socket del servidor al epoll
	if (addSocketToEpoll(epollFd, serverFd, EPOLLIN) < 0) {
		close(epollFd);
		close(serverFd);
		unlink(SOCKET_PATH);
		return 1;
	}

	cout << "[SERVER] Servidor listo, esperando conexiones...\n";
	cout << "[SERVER] Esperando comandos: búsquedas, ADD_MATCH, o EXIT\n";

	// Array de eventos
	struct epoll_event events[MAX_EVENTS];

	// Bucle principal
	while (server_running) {
		// Esperar eventos (bloqueante hasta que llegue algo)
		int nfds = epoll_wait(epollFd, events, MAX_EVENTS, -1);

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
				if (messageReady == 1) {
					// Obtener el mensaje del cliente
					string message = client_buffers[connectionFd];
					message = trim(message);
					if (!message.empty() && message.back() == '\r') 
						message.pop_back();

					cout << "[SERVER] Mensaje recibido: '" << message << "'\n";

					// 
					// COMANDO EXIT para Cerrar servidor
					// 
					if (message == "EXIT") {
						cout << "\n[SERVER]Comando EXIT recibido\n";
						cout << "[SERVER] Cerrando servidor...\n";
						
						string farewell = "SERVER_CLOSING\n";
						client_buffers[connectionFd] = farewell;
						sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
						
						server_running = false;
						break;
					}

					// 
					// COMANDO ADD_MATCH - Añadir nueva partida
					// 
					if (message.find("ADD_MATCH:") == 0) {
						cout << "\n[SERVER] Comando ADD_MATCH recibido\n";
						
						// Extraer los nombres de los jugadores
						string players_csv = message.substr(10); // Remover "ADD_MATCH:"
						cout << "[SERVER] Jugadores a añadir: " << players_csv << "\n";
						
						// Generar linea CSV completa
						string csv_line = generateCSVLine(players_csv);
						
						if (csv_line.empty()) {
							client_buffers[connectionFd] = "ERROR: No se pudieron procesar los nombres de jugadores\n";
							sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
							continue;
						}

						// Añadir al dataset y reconstruir índice
						bool success = add_line_and_rebuild(csv_line);
						
						if (success) {
							cout << "[SERVER] Partida añadida y índice reconstruido exitosamente\n";
							client_buffers[connectionFd] = "ADDED_OK\n";
						} else {
							cout << "[SERVER] Error al añadir partida\n";
							client_buffers[connectionFd] = "ERROR: No se pudo añadir la partida al dataset\n";
						}
						
						sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
						continue;
					}

					// 
					// Buscar jugador
					// 
					cout << "[SERVER] Realizando búsqueda para: " << message << "\n";
					client_buffers[connectionFd] = searchServer(message);
					sendToClient(connectionFd, client_buffers[connectionFd], epollFd);

				} else if (messageReady == -1) {
					client_buffers.erase(connectionFd);
				}
			}

			if (event & EPOLLOUT) {
				sendToClient(connectionFd, client_buffers[connectionFd], epollFd);
			}
		}

		if (!server_running) break;
	}

	// Limpieza
	cout << "\n[SERVER] Realizando limpieza...\n";
	close(epollFd);
	close(serverFd);
	unlink(SOCKET_PATH);

	cout << "[SERVER] Servidor cerrado correctamente.\n";
	return 0;
}