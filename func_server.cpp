#include "func_server.hpp"
#include <cstdio>

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

// Función para leer la línea del dataset
bool read_csv_line_at(ifstream &csv, uint64_t off, string &out) {
  csv.clear();
  csv.seekg((long long)off,
            ios::beg); // Usamos seekg para no cargar a memoria el dataset
                       // entero, solamente leer lo que nos importa
  if (!csv.good())
    return false;
  if (!getline(csv, out))
    return false; // Guardamos en la string out que pasamos por referencia el
                  // valor de retorno
  if (!out.empty() && out.back() == '\r')
    out.pop_back();
  return true;
}

// búsqueda binaria para el límite inferior
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

// búsqueda binaria para el limite superior
int64_t upper_bound_hash(ifstream &idx, uint16_t target, uint64_t N,
                         uint64_t L) {
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

string searchServer(string summoner_name) {
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
        return "ERROR: Jugador no encontrado en el índice\n";
    }
    
    int64_t R = upper_bound_hash(idx, h, N, L);
    
    // Recolectar todas las líneas CSV
    vector<string> csv_lines;
    for (int64_t i = L; i < R; ++i) {
        idx.seekg(i * 10, ios::beg);
        
        Entry e;
        if (!idx.read((char *)&e.hash16, sizeof(e.hash16))) {
            continue;
        }
        if (!idx.read((char *)&e.offset, sizeof(e.offset))) {
            continue;
        }
        
        string line;
        if (read_csv_line_at(csv, e.offset, line)) {
            csv_lines.push_back(line);
        }
    }
    
    if (csv_lines.empty()) {
        return "ERROR: No se pudieron leer líneas del CSV\n";
    }
    
    // Crear un solo proceso Python
    int pipe_to_python[2];
    int pipe_from_python[2];
    
    if (pipe(pipe_to_python) == -1 || pipe(pipe_from_python) == -1) {
        return "ERROR: No se pudieron crear pipes\n";
    }
    
    pid_t pid = fork();
    
    if (pid == -1) {
        close(pipe_to_python[0]);
        close(pipe_to_python[1]);
        close(pipe_from_python[0]);
        close(pipe_from_python[1]);
        return "ERROR: No se pudo hacer fork\n";
    }
    
    if (pid == 0) {
        // PROCESO HIJO - Python
        close(pipe_to_python[1]);
        close(pipe_from_python[0]);
        
        dup2(pipe_to_python[0], STDIN_FILENO);
        dup2(pipe_from_python[1], STDOUT_FILENO);
        
        close(pipe_to_python[0]);
        close(pipe_from_python[1]);
        
        execlp("python3", "python3", "process_matches.py", summoner_name.c_str(), nullptr);
        
        exit(1);
    }
    
    // PROCESO PADRE
    close(pipe_to_python[0]);
    close(pipe_from_python[1]);
    
    // Enviar todas las líneas a Python
    for (const auto& line : csv_lines) {
        write(pipe_to_python[1], line.c_str(), line.length());
        write(pipe_to_python[1], "\n", 1);
    }
    close(pipe_to_python[1]);
    
    // Leer todo el resultado
    string all_results = "";
    char buffer[8192];
    ssize_t bytes_read;
    
    while ((bytes_read = read(pipe_from_python[0], buffer, sizeof(buffer))) > 0) {
        all_results.append(buffer, bytes_read);
    }
    
    close(pipe_from_python[0]);
    
    int status;
    waitpid(pid, &status, 0);
    
    if (all_results.empty() || all_results.find("ERROR") == 0) {
        return "ERROR: No se encontraron partidas del jugador\n";
    }
    
    return all_results;
}

void recieveRequest(const char *request_pipe, char *buffer) {
  int fd_req =
      open(request_pipe,
           O_RDONLY); // abrimos la pipe para recibir requests del cliente
  if (fd_req < 0) {
    cerr << "[SERVER] Error abriendo pipe de solicitudes\n";
    return;
  }
  ssize_t bytes_read =
      read(fd_req, buffer, 255); // Esperamos a que haya algo en la pipe (hasta
                                 // que no haya algo no sigue el código)
  if (bytes_read > 0) {
    buffer[bytes_read] = '\0';
  }
  close(fd_req);
}

void sendResult(const char *response_pipe, string &result) {
  int fd_resp = open(response_pipe,
                     O_WRONLY); // Abrimos la pipe para responder en modo
                                // escritura y escribimos ahí la respuesta
                                // usando como buffer la string result
  if (fd_resp < 0) {
    cerr << "[SERVER] Error abriendo pipe de respuesta\n";
  }

  write(fd_resp, result.c_str(), result.size());
  close(fd_resp);

  cout << "[SERVER] Resultado enviado.\n";
}