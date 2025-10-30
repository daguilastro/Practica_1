#include "func_server.hpp"
#include <cstdio>
#include <string>

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
map<int, string> client_buffers;
map<int, size_t> client_out_bytes;
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

string searchServer(string &summoner_name) {
	summoner_name.pop_back();
	cout << "[SERVER] Jugador a buscar " << summoner_name << "\n";
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
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
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

	if (epoll_ctl(fdEpoll, EPOLL_CTL_ADD, fdSocket, &event) != 0) {
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

int acceptNewClient(int fdServer, int fdEpoll) {
	struct sockaddr_un client_addr;
	socklen_t client_len = sizeof(client_addr);

	int clientFd = accept(fdServer, (struct sockaddr *)&client_addr, &client_len); // Se acepta la conexión pendiente en el file descriptor del server

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

int receiveFromClient(int clientFd, int epollFd) {
	char buffer[BUFFER_SIZE];
	while (true) {
		ssize_t bytes_recv = recv(clientFd, buffer, sizeof(buffer), 0);
		if (bytes_recv > 0) { // Hay datos
			client_buffers[clientFd].append(buffer, bytes_recv);
			continue;
		} else if (bytes_recv == 0) { // Se cerró la conexión
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
			if (client_buffers[clientFd].find('\n') == string::npos) {
				return -1;
			}
			return 1;
		} else if (errno == EAGAIN) {
			return client_buffers[clientFd].find('\n') == string::npos ? 0 : 1;
		} else {
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, nullptr);
			close(clientFd);
			return -1;
		}
	}
}

void sendToClient(int clientFd, string &data, int epollFd) {
	const char *ptr = data.c_str();
	size_t &totalSent = client_out_bytes[clientFd];
	cout << "[SERVIDOR] la respuesta pesa: " << data.size() << "\n";
	fflush(stdout);
	while (totalSent < data.size()) {
		ssize_t bytes_sent = send(clientFd, ptr + totalSent, data.size() - totalSent, 0);
		cout << "[SERVIDOR] ENVIÉ BYTES: " << bytes_sent << "\n";
		fflush(stdout);
		if (bytes_sent < 0) {
			if (errno == EAGAIN) {
				epoll_event ev {};
				ev.data.fd = clientFd;
				ev.events = EPOLLOUT | EPOLLIN;
				epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev); // prender epollout
				return; //
			}
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
			close(clientFd);
			cerr << "[ERROR] Error enviando datos\n";
			return;
		}
		if (bytes_sent > 0) {
			totalSent += bytes_sent;
			if (totalSent == data.size()) {
				cout << "[SERVIDOR] EL TOTAL ENVIADO IGUALA EL TAMAÑO: " << totalSent << "\n";
				fflush(stdout);
				epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
				close(clientFd);
				client_out_bytes.erase(clientFd);
				client_buffers.erase(clientFd);
				return; // Ya envió todo
			}
			continue;
		}
	}
	return;
}

// ============================================
// UTILIDADES
// ============================================

string trim(const string &s) {
	string result = s;

	// Eliminar del final
	while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\r' || result.back() == '\n')) {
		result.pop_back();
	}

	// Eliminar del inicio
	size_t i = 0;
	while (i < result.size() && (result[i] == ' ' || result[i] == '\t' || result[i] == '\r' || result[i] == '\n')) {
		++i;
	}

	return result.substr(i);
}

bool add_line_and_rebuild(const string &csv_line) {
    cout << "[SERVER] add_line_and_rebuild: escribiendo línea en dataset.csv\n";
    string to_write = csv_line;
    if (to_write.empty()) {
        cerr << "[SERVER] add_line_and_rebuild: línea vacía, abortando\n";
        return false;
    }
    if (to_write.back() != '\n') to_write.push_back('\n');

    ofstream out("dataset.csv", ios::out | ios::app);
    if (!out.is_open()) {
        cerr << "[SERVER] add_line_and_rebuild: no pude abrir dataset.csv para append\n";
        return false;
    }
    out << to_write;
    if (!out) {
        cerr << "[SERVER] add_line_and_rebuild: error escribiendo en dataset.csv\n";
        out.close();
        return false;
    }
    out.close();

    cout << "[SERVER] Línea añadida (preview):\n";
    if (to_write.size() <= 200) cout << to_write;
    else cout << to_write.substr(0,200) << "...\n";

    cout << "[SERVER] Ejecutando ./build_sorted_index ...\n";
    int sysret = system("./build_sorted_index");
    if (sysret != 0) {
        cerr << "[SERVER] build_sorted_index devolvió código: " << sysret << "\n";
        return false;
    }
    cout << "[SERVER] build_sorted_index finalizó OK\n";
    return true;
}