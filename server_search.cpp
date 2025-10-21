#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdint>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace std;

struct Entry {      // La struct de como están ordenados los archivos en la tabla hash
    uint16_t hash16;
    uint64_t offset;
};

uint32_t HASH_MOD = 65536;

// Funciones para hacer el hash y reducirlo
uint32_t fnv1a32(const string& s){
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = s[i];
        h ^= c;
        h *= 16777619u;
    }
    return h;
}
uint16_t hash_reduce(const string& s){
    return (uint16_t)(fnv1a32(s) % HASH_MOD);
}

// Función para modificar strings
string trim_copy(string s){
    while(!s.empty() && (s.back()==' ' || s.back()=='\t' || s.back()=='\r' || s.back()=='\n')) {
        s.pop_back();
    }
    size_t i = 0;
    while (i < s.size() && (s[i]==' ' || s[i]=='\t' || s[i]=='\r' || s[i]=='\n')) {
        ++i;
    }
    return s.substr(i);
}

// Función para leer la línea del dataset
bool read_csv_line_at(ifstream& csv, uint64_t off, string& out){
    csv.clear();
    csv.seekg((long long)off, ios::beg);    // Usamos seekg para no cargar a memoria el dataset entero, solamente leer lo que nos importa
    if(!csv.good()) return false;
    if(!getline(csv, out)) return false;    // Guardamos en la string out que pasamos por referencia el valor de retorno
    if(!out.empty() && out.back() == '\r') out.pop_back();
    return true;
}

double extract_game_duration_seconds(const string& line){
    size_t i = 0;
    size_t n = line.size();
    
    while(i < n && line[i] != ',') ++i;
    if(i < n) ++i;
    
    while(i < n && line[i] != ',') ++i;
    if(i < n) ++i;
    
    size_t start = i;
    while(i < n && line[i] != ',') ++i;
    
    string duration_str = line.substr(start, i - start);
    
    double result = 0.0;
    try {
        result = stod(duration_str);
    } catch(...) {
        result = 0.0;
    }
    
    return result;
}

string extract_summoner_names_as_string(const string& line) {
    string result = "";
    const string pattern = "'summonerName': '";
    size_t pos = 0;
    int count = 0;

    while (count < 10 && pos < line.size()) {
        size_t found = line.find(pattern, pos);
        if (found == string::npos) break;
        
        size_t start = found + pattern.size();
        size_t end = line.find('\'', start);
        string name = line.substr(start, end - start);
        
        result += "  " + to_string(count + 1) + ". " + name + "\n";
        
        pos = end + 1;
        count++;
    }

    return result;
}

string get_match_result(const string& line, const string& player_name) {
    size_t q1 = line.find('\"');
    if (q1 == string::npos) return "Perdió";
    size_t q2 = line.find('\"', q1 + 1);
    if (q2 == string::npos) return "Perdió";
    size_t q3 = line.find('\"', q2 + 1);
    if (q3 == string::npos) return "Perdió";
    size_t q4 = line.find('\"', q3 + 1);
    if (q4 == string::npos) return "Perdió";

    string identities = line.substr(q1 + 1, q2 - (q1 + 1));
    string parts      = line.substr(q3 + 1, q4 - (q3 + 1));

    const string NAME_KEY = "'summonerName': '";
    const string PID_KEY  = "'participantId': ";

    size_t name_pos = identities.find(NAME_KEY + player_name + "'");
    if (name_pos == string::npos) return "Perdió";

    size_t pid_pos = identities.rfind(PID_KEY, name_pos);
    if (pid_pos == string::npos) return "Perdió";
    pid_pos += PID_KEY.size();

    size_t pid_end = pid_pos;
    while (pid_end < identities.size() && isdigit((unsigned char)identities[pid_end])) ++pid_end;
    if (pid_end == pid_pos) return "Perdió";

    int pid = stoi(identities.substr(pid_pos, pid_end - pid_pos));

    string pid_token = "'participantId': " + to_string(pid);
    size_t p = parts.find(pid_token);
    if (p == string::npos) return "Perdió";

    size_t sep_next = parts.find("}, {", p);
    size_t list_end = parts.find("}]",   p);
    size_t block_end;
    if (sep_next == string::npos && list_end == string::npos) block_end = parts.size();
    else if (sep_next == string::npos) block_end = list_end;
    else if (list_end == string::npos) block_end = sep_next;
    else block_end = min(sep_next, list_end);

    const string WIN_KEY = "'win':";
    size_t w = parts.find(WIN_KEY, p);
    if (w == string::npos || w > block_end) return "Perdió";

    size_t v = w + WIN_KEY.size();
    while (v < block_end && (parts[v] == ' ' || parts[v] == '\t')) ++v;

    if (v + 4 <= block_end && parts.compare(v, 4, "True") == 0)  return "Ganó";
    if (v + 5 <= block_end && parts.compare(v, 5, "False") == 0) return "Perdió";

    return "Perdió";
}

// búsqueda binaria para el límite inferior
int64_t lower_bound_hash(ifstream& idx, uint16_t target, uint64_t N){
    uint64_t lo = 0;
    uint64_t hi = N;
    
    while(lo < hi){
        uint64_t mid = (lo + hi) / 2;
        idx.seekg(mid * 10, ios::beg);
        
        Entry e;
        if(!idx.read((char*)&e.hash16, sizeof(e.hash16))) return -1;
        if(!idx.read((char*)&e.offset, sizeof(e.offset))) return -1;
        
        if(e.hash16 < target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    
    if(lo >= N) return -1;
    
    idx.seekg(lo * 10, ios::beg);
    Entry e;
    if(!idx.read((char*)&e.hash16, sizeof(e.hash16))) return -1;
    if(!idx.read((char*)&e.offset, sizeof(e.offset))) return -1;
    
    return (e.hash16 == target) ? (int64_t)lo : -1;
}

// búsqueda binaria para el limite superior
int64_t upper_bound_hash(ifstream& idx, uint16_t target, uint64_t N, uint64_t L){
    uint64_t lo = L;
    uint64_t hi = N;
    
    while(lo < hi){
        uint64_t mid = (lo + hi) / 2;
        idx.seekg(mid * 10, ios::beg);
        
        Entry e;
        if(!idx.read((char*)&e.hash16, sizeof(e.hash16))) return -1;
        if(!idx.read((char*)&e.offset, sizeof(e.offset))) return -1;
        
        if(e.hash16 <= target) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    
    return (int64_t)lo;
}

// Buscamos en la hash table usando búsqueda binaria para más estilo
string search_player(const string& name) {
    string result = "";
    
    if(name.empty()){
        return "NA\n";
    }
    
    uint16_t h = hash_reduce(name); // hasheamos el nombre

    ifstream idx("index_sorted.idx", ios::binary);  // abrimos como stream la tabla hash en modo binario. (nada se carga en memoria aún)
    if(!idx){
        return "ERROR: No pude abrir index_sorted.idx\n";
    }
    
    ifstream csv("dataset.csv", ios::binary);   // Hacemos lo mismo con el dataset
    if(!csv){
        return "ERROR: No pude abrir dataset.csv\n";
    }

    idx.seekg(0, ios::end); // mueve el puntero al final del archivo
    uint64_t bytes = idx.tellg();   // dice la cantidad de bytes totales (da la ubicación del puntero que está al final)
    uint64_t N = bytes / 10;    // calcula cuantas entradas hay porque cada entrada pesa 10 bytes (hash y offset)


    // Encuentra los límites de los hashes (donde inicia y donde termina)
    int64_t L = lower_bound_hash(idx, h, N);
    if(L < 0){
        return "NA\n";
    }
    
    int64_t R = upper_bound_hash(idx, h, N, L);

    bool found_any = false;

    for(int64_t i = L; i < R; ++i){
        idx.seekg(i * 10, ios::beg);    // vamos a la posición donde esta la primer coincidencia del hash
        
        Entry e;
        if(!idx.read((char*)&e.hash16, sizeof(e.hash16))) {
            return "ERROR: No se pudo leer el hash del índice\n";
        }
        if(!idx.read((char*)&e.offset, sizeof(e.offset))) {
            return "ERROR: No se pudo leer el offset del índice\n";
        }

        string line;
        if(!read_csv_line_at(csv, e.offset, line)) {
            return "ERROR: No se pudo leer la línea del CSV\n";
        }
        if(line.find(name) == string::npos) continue;   // si no encuentra el nombre continua a la siguiente posición (hay que recordar que hay colisiones es por esto que se hace esta verificación)

        //guardamos los datos de la partida
        double d = extract_game_duration_seconds(line); 
        int mins = (int)(d / 60.0);
        int secs = (int)round(fmod(d, 60.0));
        
        string players = extract_summoner_names_as_string(line);
        string match_result = get_match_result(line, name);

        // lo guardamos todo en result
        result += "\n================= PARTIDA =================\n";
        result += "Jugador buscado : " + name + "\n";
        result += "Duracion        : " + to_string(mins) + " min ";
        if(secs < 10) result += "0";
        result += to_string(secs) + " s\n";
        result += "Resultado       : " + match_result + "\n";
        result += "Participantes   :\n";
        result += players;
        result += "(offset CSV: " + to_string(e.offset) + ")\n";
        result += "===========================================\n";
        result += "NEXT_MATCH\n";  // Marcador para separar partidas (MUY IMPORTANTE JAJAJA)

        found_any = true;
    }

    if(!found_any){ // si no se encuentra ninguna coincidencia retorna NA
        return "NA\n";
    }
    
    result += "END_SEARCH\n";  // Marcador de fin de búsqueda
    return result;
}

int main(){
    // las direcciones de ambas pipes
    const char* request_pipe = "/tmp/search_request";
    const char* response_pipe = "/tmp/search_response";
    
    // Crear named pipes si no existen
    mkfifo(request_pipe, 0666); // 0666 son permisos en octal aquí creamos como tal las pipes
    mkfifo(response_pipe, 0666);
    
    cout << "[SERVER] Servidor de búsqueda iniciado. Esperando solicitudes...\n";
    
    while(true){
        // Abrir pipe de solicitudes (bloqueante hasta que cliente conecte)
        int fd_req = open(request_pipe, O_RDONLY);  // abrimos la pipe para recibir requests del cliente
        if(fd_req < 0){
            cerr << "[SERVER] Error abriendo pipe de solicitudes\n";
            continue;
        }
        
        char buffer[256];   // creamos un buffer para leer el nombre escrito
        memset(buffer, 0, sizeof(buffer));  // lo seteamos a 0 por si había algo
        
        ssize_t bytes_read = read(fd_req, buffer, sizeof(buffer) - 1);  // Esperamos a que haya algo en la pipe (hasta que no haya algo no sigue el código)
        close(fd_req);
        
        if(bytes_read <= 0) continue;
        
        string name = trim_copy(string(buffer));
        
        cout << "[SERVER] Búsqueda recibida: \"" << name << "\"\n";
        
        // Verificar comando de salida
        if(name == "EXIT_SERVER"){
            cout << "[SERVER] Comando de salida recibido. Cerrando servidor...\n";
            break;
        }
        
        // Realizar búsqueda
        string result = search_player(name);
        
        // Enviar resultado
        int fd_resp = open(response_pipe, O_WRONLY);    // Abrimos la pipe para responder en modo escritura y escribimos ahí la respuesta usando como buffer la string result
        if(fd_resp < 0){
            cerr << "[SERVER] Error abriendo pipe de respuesta\n";
            continue;
        }
        
        write(fd_resp, result.c_str(), result.size());
        close(fd_resp);
        
        cout << "[SERVER] Resultado enviado.\n";
    }
    
    // Limpiar pipes
    unlink(request_pipe);
    unlink(response_pipe);
    
    return 0;
}