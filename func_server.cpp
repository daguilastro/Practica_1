#include "func_server.hpp"
#include <cstdio>
#include <string>

// 
// FUNCIONES DE HASH
// 
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


// 
// FUNCIONES DE AÑADIR PARTIDAS
// 
string generateCSVLine(const string &players_csv) {
    // Separar los nombres por comas
    vector<string> players;
    stringstream ss(players_csv);
    string player;
    
    while (getline(ss, player, ',')) {
        player = trim(player);
        if (!player.empty()) {
            players.push_back(player);
        }
    }
    
    if (players.size() != 10) {
        cerr << "[SERVER] Error: Se requieren exactamente 10 jugadores, y se ingresaron solo: " << players.size() << "\n";
        return "";
    }
    
    // Generar timestamp actual
    time_t now = time(nullptr);
    uint64_t timestamp = (uint64_t)now * 1000; // Convertir a milisegundos
    
    // Generar IDs aleatorios
    random_device rd;
    mt19937_64 gen(rd());
    uniform_int_distribution<uint64_t> dist(1000000000, 9999999999);
    
    uint64_t matchId = dist(gen);
    uint64_t gameId = dist(gen);
    
    // Construir seccion de identidades (todos los jugadores)
    stringstream identities;
    identities << "[";
    for (int i = 0; i < 10; i++) {
        if (i > 0) identities << ", ";
        identities << "{'participantId': " << (i+1) 
                  << ", 'player': {"
                  << "'platformId': 'KR', "
                  << "'accountId': 'acc" << dist(gen) << "', "
                  << "'summonerName': '" << players[i] << "', "
                  << "'summonerId': 'sum" << dist(gen) << "', "
                  << "'currentPlatformId': 'KR', "
                  << "'currentAccountId': 'acc" << dist(gen) << "', "
                  << "'matchHistoryUri': '/v1/stats/player_history/KR/" << dist(gen) << "', "
                  << "'profileIcon': " << (1000 + i) << "}}";
    }
    identities << "]";
    
    // Construir seccion de participantes (estadísticas simuladas)
    stringstream participants;
    participants << "[";
    for (int i = 0; i < 10; i++) {
        if (i > 0) participants << ", ";
        
        int teamId = (i < 5) ? 100 : 200; // Primeros 5 = equipo azul, últimos 5 = equipo rojo
        bool win = (teamId == 100); // Equipo azul gana
        
        // Generar stadisticas aleatorias pero realistas
        int kills = (rand() % 10);
        int deaths = (rand() % 8) + 1;
        int assists = (rand() % 15);
        int cs = 50 + (rand() % 200);
        int gold = 3000 + (rand() % 10000);
        int damage = 5000 + (rand() % 20000);
        
        participants << "{'participantId': " << (i+1) 
                    << ", 'teamId': " << teamId
                    << ", 'championId': " << (1 + rand() % 150)
                    << ", 'spell1Id': " << (4 + rand() % 8)
                    << ", 'spell2Id': " << (4 + rand() % 8)
                    << ", 'stats': {"
                    << "'participantId': " << (i+1)
                    << ", 'win': " << (win ? "True" : "False")
                    << ", 'kills': " << kills
                    << ", 'deaths': " << deaths
                    << ", 'assists': " << assists
                    << ", 'totalMinionsKilled': " << cs
                    << ", 'neutralMinionsKilled': " << (rand() % 50)
                    << ", 'goldEarned': " << gold
                    << ", 'totalDamageDealtToChampions': " << damage
                    << ", 'totalDamageTaken': " << (damage / 2)
                    << ", 'visionScore': " << (10 + rand() % 50)
                    << ", 'champLevel': " << (6 + rand() % 12)
                    << ", 'item0': " << (rand() % 4000)
                    << ", 'item1': " << (rand() % 4000)
                    << ", 'item2': " << (rand() % 4000)
                    << ", 'item3': " << (rand() % 4000)
                    << ", 'item4': " << (rand() % 4000)
                    << ", 'item5': " << (rand() % 4000)
                    << ", 'item6': " << 3340
                    << "}, 'timeline': {'participantId': " << (i+1)
                    << ", 'role': 'DUO_SUPPORT', 'lane': 'NONE'}}";
    }
    participants << "]";
    
    // Construir linea CSV completa
    stringstream csv_line;
    csv_line << matchId << ","
             << timestamp << ".0,"
             << (600.0 + (rand() % 1200)) << ".0," // Duración (10-30 minutos)
             << gameId << ".0,"
             << "CLASSIC,MATCHED_GAME,10.2.305.4739,11.0,"
             << "\"" << identities.str() << "\","
             << "\"" << participants.str() << "\","
             << "KR,420.0,13.0,,";
    
    return csv_line.str();
}

// Funcion para añadir línea al dataset y reconstruir el índice
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

    cout << "[SERVER] Línea añadida (preview primeros 200 chars):\n";
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

// 
// FUNCIONES DE BÚSQUEDA EN DATASET
// 
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
	if (!summoner_name.empty() && summoner_name.back() == '\n') {
		summoner_name.pop_back();
	}
	cout << "[SERVER] Jugador a buscar: " << summoner_name << "\n";
	
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

	// Aumentar tamaño de buffer de las pipes
	int buffer_size = 1024 * 1024;
	fcntl(pipe_to_python[1], F_SETPIPE_SZ, buffer_size);
	fcntl(pipe_from_python[1], F_SETPIPE_SZ, buffer_size);

	pid_t pid = fork();

	// Por si da error
	if (pid == -1) {
		close(pipe_to_python[0]);
		close(pipe_to_python[1]);
		close(pipe_from_python[0]);
		close(pipe_from_python[1]);
		return "ERROR: No se pudo hacer fork\n";
	}

	if (pid == 0) {
		// Proceso hijo (Python)
		close(pipe_to_python[1]);
		close(pipe_from_python[0]);

		string input_fd_str = to_string(pipe_to_python[0]);
		string output_fd_str = to_string(pipe_from_python[1]);

		setenv("INPUT_FD", input_fd_str.c_str(), 1);
		setenv("OUTPUT_FD", output_fd_str.c_str(), 1);

		execlp("python3", "python3", "process_matches.py", summoner_name.c_str(), nullptr);

		cerr << "[ERROR] No se pudo ejecutar Python\n";
		exit(1);
	}

	// Proceso padre
	close(pipe_to_python[0]);
	close(pipe_from_python[1]);

	string all_data_to_send = "";
	for (const auto &line : csv_lines) {
		all_data_to_send += line + "\n";
	}
	write(pipe_to_python[1], all_data_to_send.c_str(), all_data_to_send.length());
	close(pipe_to_python[1]);

	string all_results = "";
	char buffer[1048576];
	ssize_t bytes_read;

	while ((bytes_read = read(pipe_from_python[0], buffer, sizeof(buffer))) > 0) {
		all_results.append(buffer, bytes_read);
	}

	close(pipe_from_python[0]);

	// Esperar a que el proceso hijo termine
	int status;
	waitpid(pid, &status, 0);

	if (all_results.empty() || all_results.find("ERROR") == 0) {
		return "NA\n";
	}

	return all_results;
}

// 
// FUNCIONES DE SOCKET Y EPOLL
// 
void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags >= 0) {
		fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	}
}

int createNBSocket() {
	// Aqui se crea el socket UNIX
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		cerr << "[ERROR] No se pudo crear socket\n";
		return -1;
	}

	// Para hacerlo no bloqueante
	setNonBlocking(fd);

	// Se configura la direccion
	struct sockaddr_un addr;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	// Se elimina el socket viejo si existia
	unlink(SOCKET_PATH);

	// bind
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
		cerr << "[ERROR] Error en bind\n";
		close(fd);
		return -1;
	}

	// listen
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


// 
// FUNCIONES DE MANEJO DE CLIENTES
// 
int acceptNewClient(int fdServer, int fdEpoll) {
	struct sockaddr_un client_addr;
	socklen_t client_len = sizeof(client_addr);

	int clientFd = accept(fdServer, (struct sockaddr *)&client_addr, &client_len);

	if (clientFd < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			// No hay mas conexiones pendientes
			return 0;
		}
		cerr << "[ERROR] Error en accept\n";
		return -1;
	}

	cout << "[SERVER] Cliente conectado (FD " << clientFd << ")\n";

	// Hacer el cliente no bloqueante y agregarlo a epoll
	setNonBlocking(clientFd);
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
		if (bytes_recv > 0) { // hay datos
			client_buffers[clientFd].append(buffer, bytes_recv);
			continue;
		} else if (bytes_recv == 0) { // se ceerró la conexión
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
	
	while (totalSent < data.size()) {
		ssize_t bytes_sent = send(clientFd, ptr + totalSent, data.size() - totalSent, 0);
		
		if (bytes_sent < 0) {
			if (errno == EAGAIN) {
				epoll_event ev {};
				ev.data.fd = clientFd;
				ev.events = EPOLLOUT | EPOLLIN;
				epoll_ctl(epollFd, EPOLL_CTL_MOD, clientFd, &ev); // Prender epollout
				return;
			}
			epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
			close(clientFd);
			cerr << "[ERROR] Error enviando datos\n";
			return;
		}
		if (bytes_sent > 0) {
			totalSent += bytes_sent;
			if (totalSent == data.size()) {
				epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
				close(clientFd);
				client_out_bytes.erase(clientFd);
				client_buffers.erase(clientFd);
				return; // Ya envio todo
			}
			continue;
		}
	}
	return;
}


// 
// UTILIDADES
// 
string trim(const string &s) {
	string result = s;

	// Limpiar espacios al final
	while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\r' || result.back() == '\n')) {
		result.pop_back();
	}

	// Limpiar espacios al inicio
	size_t i = 0;
	while (i < result.size() && (result[i] == ' ' || result[i] == '\t' || result[i] == '\r' || result[i] == '\n')) {
		++i;
	}

	return result.substr(i);
}