#include "func_server.hpp"

int main() {
    // 1. Crear socket del servidor
    int fdServer = createNBSocket();
    if (fdServer < 0) {
        cerr << "[ERROR] No se pudo crear socket del servidor\n";
        return 1;
    }

    // 2. Crear instancia de epoll
    int fdEpoll = epoll_create1(0);
    if (fdEpoll < 0) {
        cerr << "[ERROR] No se pudo crear epoll\n";
        close(fdServer);
        unlink(SOCKET_PATH);
        return 1;
    }

    cout << "[SERVER] Epoll creado correctamente\n";

    // 3. Agregar socket del servidor al epoll
    if (addSocketToEpoll(fdEpoll, fdServer, EPOLLIN) < 0) {
        close(fdEpoll);
        close(fdServer);
        unlink(SOCKET_PATH);
        return 1;
    }

    cout << "[SERVER] Servidor listo, esperando conexiones...\n";

    // 4. Array de eventos
    struct epoll_event events[MAX_EVENTS];

    // 5. Bucle principal
    while (true) {
        // Esperar eventos (bloqueante hasta que llegue algo)
        int nfds = epoll_wait(fdEpoll, events, MAX_EVENTS, -1); // Number of file descriptors

        if (nfds < 0) {
            cerr << "[ERROR] Error en epoll_wait\n";
            break;
        }

        // Procesar todos los eventos
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            // Evento en el servidor = nueva conexión
            if (fd == fdServer) {
                // Aceptar todas las conexiones pendientes
                while (true) {
                    int clientFd = acceptNewClient(fdServer, fdEpoll);
                    if (clientFd <= 0) {
                        break;  // No hay más conexiones o error
                    }
                }
            }
            // Evento en un cliente = datos recibidos
            else {
                handleClientData(fd, fdEpoll);
            }
        }
    }

    // 6. Limpieza
    close(fdEpoll);
    close(fdServer);
    unlink(SOCKET_PATH);

    cout << "[SERVER] Servidor cerrado\n";
    return 0;
}