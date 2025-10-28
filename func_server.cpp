#include "func_server.hpp"

// ============================================
// FUNCIONES DE HASH
// ============================================

uint32_t fnv1a32(const string &s, uint32_t HASH_MOD) {
    const uint32_t FNV_PRIME = 16777619u;
    const uint32_t FNV_OFFSET = 2166136261u;

    uint32_t hash = FNV_OFFSET;
    for (char c : s) {
        hash ^= (uint8_t)c;
        hash *= FNV_PRIME;
    }

    return hash % HASH_MOD;
}

// ============================================
// FUNCIONES DE BÚSQUEDA EN DATASET
// ============================================

bool read_csv_line_at(ifstream &csv, uint64_t off, string &out) {
    csv.clear();
    csv.seekg((long long)off, ios::beg);
    if (!csv.good())
        return false;
    if (!getline(csv, out))
        return false;
    if (!out.empty() && out.back() == '\r')
        out.pop_back();
    return true;
}

int64_t lower_bound_hash(ifstream &idx, uint16_t target, uint64_t N) {
    uint64_t lo = 0;
    uint64_t hi = N;

    while (lo < hi) {
        uint64_t mid = (lo + hi) / 2;
        idx.seekg(mid * 10, ios::beg);

        Entry e;
        if (!idx.read((char *)&e.hash16, sizeof(e.hash16)))
            return -1;
        if (!idx.read((char *)&e.offset, sizeof(e.offset)))
            return -1;

        if (e.hash16 < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo >= N)
        return -1;

    idx.seekg(lo * 10, ios::beg);
    Entry e;
    if (!idx.read((char *)&e.hash16, sizeof(e.hash16)))
        return -1;
    if (!idx.read((char *)&e.offset, sizeof(e.offset)))
        return -1;

    return (e.hash16 == target) ? (int64_t)lo : -1;
}

int64_t upper_bound_hash(ifstream &idx, uint16_t target, uint64_t N, uint64_t L) {
    uint64_t lo = L;
    uint64_t hi = N;

    while (lo < hi) {
        uint64_t mid = (lo + hi) / 2;
        idx.seekg(mid * 10, ios::beg);

        Entry e;
        if (!idx.read((char *)&e.hash16, sizeof(e.hash16)))
            return -1;
        if (!idx.read((char *)&e.offset, sizeof(e.offset)))
            return -1;

        if (e.hash16 <= target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    return (int64_t)lo;
}

string searchServer(const string &summoner_name) {
    uint16_t h = fnv1a32(summoner_name, HASH_MOD);

    ifstream idx("index_sorted.idx", ios::binary);
    if (!idx) {
        return "ERROR: No pude abrir index_sorted.idx\n";
    }

    ifstream csv("dataset.csv", ios::binary);
    if (!csv) {
        return "ERROR: No pude abrir dataset.csv\n";
    }

    idx.seekg(0, ios::end);
    uint64_t bytes = idx.tellg();
    uint64_t N = bytes / 10;

    int64_t L = lower_bound_hash(idx, h, N);
    if (L < 0) {
        return "NA\n";
    }

    int64_t R = upper_bound_hash(idx, h, N, L);

    vector<string> csv_lines;
    for (int64_t i = L; i < R; ++i) {
        idx.seekg(i * 10, ios::beg);

        Entry e;
        if (!idx.read((char *)&e.hash16, sizeof(e.hash16)))
            continue;
        if (!idx.read((char *)&e.offset, sizeof(e.offset)))
            continue;

        string line;
        if (read_csv_line_at(csv, e.offset, line)) {
            // Verificar que el nombre esté realmente en la línea (por colisiones)
            if (line.find(summoner_name) != string::npos) {
                csv_lines.push_back(line);
            }
        }
    }

    if (csv_lines.empty()) {
        return "NA\n";
    }

    // Crear pipes para comunicación con Python
    int pipe_to_python[2];
    int pipe_from_python[2];

    if (pipe(pipe_to_python) == -1 || pipe(pipe_from_python) == -1) {
        return "ERROR: No se pudieron crear pipes\n";
    }

    // Aumentar tamaño del buffer de las pipes
    int buffer_size = 1024 * 1024;
    fcntl(pipe_to_python[1], F_SETPIPE_SZ, buffer_size);
    fcntl(pipe_from_python[1], F_SETPIPE_SZ, buffer_size);

    pid_t pid = fork();

    if (pid == -1) {
        close(pipe_to_python[0]);
        close(pipe_to_python[1]);
        close(pipe_from_python[0]);
        close(pipe_from_python[1]);
        return "ERROR: No se pudo hacer fork\n";
    }

    if (pid == 0) {
        // PROCESO HIJO (Python)
        close(pipe_to_python[1]);
        close(pipe_from_python[0]);

        string input_fd_str = to_string(pipe_to_python[0]);
        string output_fd_str = to_string(pipe_from_python[1]);

        setenv("INPUT_FD", input_fd_str.c_str(), 1);
        setenv("OUTPUT_FD", output_fd_str.c_str(), 1);

        execlp("python3", "python3", "process_matches.py", summoner_name.c_str(), nullptr);
        
        // Si llegamos aquí, execlp falló
        cerr << "[ERROR] No se pudo ejecutar Python\n";
        exit(1);
    }

    // PROCESO PADRE
    close(pipe_to_python[0]);
    close(pipe_from_python[1]);

    // Enviar todas las líneas del CSV a Python
    string all_data_to_send = "";
    for (const auto &line : csv_lines) {
        all_data_to_send += line + "\n";
    }
    write(pipe_to_python[1], all_data_to_send.c_str(), all_data_to_send.length());
    close(pipe_to_python[1]);

    // Recibir resultados de Python
    string all_results = "";
    char buffer[1048576];
    ssize_t bytes_read;

    while ((bytes_read = read(pipe_from_python[0], buffer, sizeof(buffer))) > 0) {
        all_results.append(buffer, bytes_read);
    }

    close(pipe_from_python[0]);

    // Esperar a que termine el proceso hijo
    int status;
    waitpid(pid, &status, 0);

    if (all_results.empty() || all_results.find("ERROR") == 0) {
        return "NA\n";
    }

    return all_results;
}

// ============================================
// FUNCIONES DE SOCKET Y EPOLL
// ============================================

void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

int createNBSocket() {
    // Crear socket UNIX
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        cerr << "[ERROR] No se pudo crear socket\n";
        return -1;
    }

    // Hacerlo no bloqueante
    setNonBlocking(fd);

    // Configurar dirección
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Eliminar socket anterior si existe
    unlink(SOCKET_PATH);

    // Bind
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        cerr << "[ERROR] Error en bind\n";
        close(fd);
        return -1;
    }

    // Listen
    if (listen(fd, 30) != 0) {
        cerr << "[ERROR] Error en listen\n";
        close(fd);
        unlink(SOCKET_PATH);
        return -1;
    }

    cout << "[SERVER] Socket creado en " << SOCKET_PATH << "\n";
    return fd;
}

int addSocketToEpoll(int fdEpoll, int fdSocket, uint32_t events) {
    struct epoll_event event;
    event.events = events;
    event.data.fd = fdSocket;
    
    if (epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdSocket, &event) < 0) {
        cerr << "[ERROR] No se pudo agregar FD " << fdSocket << " a epoll\n";
        return -1;
    }
    
    return 0;
}

int removeSocketFromEpoll(int fdEpoll, int fdSocket) {
    if (epoll_ctl(fdEpoll, EPOLL_CTL_DEL, fdSocket, NULL) < 0) {
        cerr << "[ERROR] No se pudo eliminar FD " << fdSocket << " de epoll\n";
        return -1;
    }
    return 0;
}

// ============================================
// FUNCIONES DE MANEJO DE CLIENTES
// ============================================

// Buffer estático para mantener mensajes parciales por cliente
static map<int, string> client_buffers;

int acceptNewClient(int fdServer, int fdEpoll) {
    struct sockaddr_un client_addr;
    socklen_t client_len = sizeof(client_addr);

    int clientFd = accept(fdServer, (struct sockaddr*)&client_addr, &client_len); // Se acepta la conexión pendiente en el file descriptor del server

    if (clientFd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No hay más conexiones pendientes
            return 0;
        }
        cerr << "[ERROR] Error en accept\n";
        return -1;
    }

    cout << "[SERVER] Cliente conectado (FD " << clientFd << ")\n";

    // Hacer el cliente no bloqueante
    setNonBlocking(clientFd);

    // Agregar cliente al epoll
    if (addSocketToEpoll(fdEpoll, clientFd, EPOLLIN) < 0) {
        close(clientFd);
        return -1;
    }

    return clientFd;
}

string receiveFromClient(int clientFd) {
    char buffer[BUFFER_SIZE];
    
    // Leer todo lo disponible en el socket
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_recv = recv(clientFd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_recv < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No hay más datos disponibles por ahora
                break;
            }
            cerr << "[ERROR] Error recibiendo datos\n";
            return "";
        }

        if (bytes_recv == 0) {
            // Cliente cerró conexión
            return "";
        }

        // Agregar al buffer del cliente
        client_buffers[clientFd] += string(buffer, bytes_recv);
    }

    // Buscar mensaje completo (terminado en '\n')
    size_t pos = client_buffers[clientFd].find('\n');
    
    if (pos != string::npos) {
        // Mensaje completo encontrado
        string complete_msg = client_buffers[clientFd].substr(0, pos);
        client_buffers[clientFd].erase(0, pos + 1);  // Eliminar del buffer
        return complete_msg;
    }

    // Mensaje incompleto, esperar más datos
    return "";
}

bool sendToClient(int clientFd, const string &data) {
    const char* ptr = data.c_str();
    size_t total_sent = 0;
    size_t total_len = data.size();

    while (total_sent < total_len) {
        ssize_t bytes_sent = send(clientFd, ptr + total_sent, 
                                 total_len - total_sent, 0);

        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Buffer lleno, esperar un poco
                usleep(1000);
                continue;
            }
            cerr << "[ERROR] Error enviando datos\n";
            return false;
        }

        total_sent += bytes_sent;
    }

    return true;
}

void handleClientData(int clientFd, int fdEpoll) {
    // Recibir nombre del jugador
    string name = receiveFromClient(clientFd);

    if (name.empty()) {
        // Mensaje incompleto o error
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            // Error real o cliente desconectado
            cout << "[SERVER] Cliente desconectado (FD " << clientFd << ")\n";
            cleanupClient(clientFd);
            removeSocketFromEpoll(fdEpoll, clientFd);
            close(clientFd);
        }
        // Si es EAGAIN/EWOULDBLOCK, simplemente esperamos más datos
        return;
    }

    cout << "[SERVER] Búsqueda recibida de FD " << clientFd << ": \"" << name << "\"\n";

    // Verificar comando de salida
    if (name == "EXIT_SERVER") {
        cout << "[SERVER] Comando de salida recibido\n";
        cleanupClient(clientFd);
        removeSocketFromEpoll(fdEpoll, clientFd);
        close(clientFd);
        // Señal para cerrar el servidor (puedes manejarlo en main)
        return;
    }

    // Realizar búsqueda
    string result = searchServer(name);

    // Enviar resultado
    if (sendToClient(clientFd, result)) {
        cout << "[SERVER] Resultado enviado a FD " << clientFd 
             << " (" << result.size() << " bytes)\n";
    } else {
        cerr << "[SERVER] Error enviando resultado a FD " << clientFd << "\n";
    }

    // Cerrar conexión con el cliente
    cleanupClient(clientFd);
    removeSocketFromEpoll(fdEpoll, clientFd);
    close(clientFd);
}

void cleanupClient(int clientFd) {
    client_buffers.erase(clientFd);
}

// ============================================
// UTILIDADES
// ============================================

string trim(const string &s) {
    string result = s;
    
    // Eliminar del final
    while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || 
                               result.back() == '\r' || result.back() == '\n')) {
        result.pop_back();
    }
    
    // Eliminar del inicio
    size_t i = 0;
    while (i < result.size() && (result[i] == ' ' || result[i] == '\t' || 
                                 result[i] == '\r' || result[i] == '\n')) {
        ++i;
    }
    
    return result.substr(i);
}