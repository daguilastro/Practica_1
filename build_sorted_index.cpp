#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>

using namespace std;

// Struct que guardará el hash del jugador y su posición
struct Entry {
    uint16_t hash16;   // hash jugador
    uint64_t offset;   // byte offset del inicio de la fila en el CSV donde está el jugador.
};

uint32_t HASH_MOD = 65536; // Módulo para reducir el hash. (ya que no necesitamos hashes tan grandes, los reducimos usando este módulo)

// FNV-1a 32-bit. Función hash
uint32_t fnv1a32(const string& s){
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}
// Función de reducción del hash
uint16_t hash_reduce(const string& s){
    return (uint16_t)(fnv1a32(s) % HASH_MOD);
}

// Extraemos los nombres de una fila pasada como argumento.
vector<string> extract_summoner_names(const string& line) {
    vector<string> names;
    names.reserve(10);  // Como solo hay 10 jugadores en cada partida reservamos de una vez 10 espacios en names.

    const string pattern = "'summonerName': '"; // Así son los prefijos en el csv que van antes del summoner name que queremos guardar
    size_t pos = 0;

    while (names.size() < 10 && pos < line.size()) {
        // Buscar la próxima aparición del texto "'summonerName': '"
        size_t found = line.find(pattern, pos);
        if (found == string::npos) { // retorna n:pos si no encuentra posición
            break; // no hay más nombres
        }

        // Calcular dónde empieza realmente el nombre
        size_t start = found + pattern.size();

        // Buscar la comilla simple que cierra el nombre
        size_t end = line.find('\'', start);

        // Extraer el nombre y añadirlo al vector
        string name = line.substr(start, end - start);
        names.push_back(name);

        // Avanzar el puntero para seguir buscando
        pos = end + 1;
    }

    return names;
}


int main() {
    // Archivos fijos
    const char* csv_path = "dataset.csv"; // Path al dataset
    const char* out_path = "index_sorted.idx"; // Path de salida

    // Abrir CSV
    ifstream csv(csv_path, ios::binary); // Abrimos el dataset en modo binario.
    if (!csv) {
        cerr << "No se pudo abrir dataset.csv\n";
        return 1;
    }

    // Leer y descartar la cabecera
    string line;
    if (!getline(csv, line)) {
        cerr << "CSV vacío o sin cabecera\n";
        return 1;
    }

    // Aquí guardaremos todas las entries
    vector<Entry> entries;

    unsigned long long rows = 0; // Aquí vamos contando cuantas rows leemos
    while (true) {
        streampos row_off = csv.tellg();   // byte donde empieza esta fila tellg() retorna el byte donde está el puntero de lectura del archivo csv
        if (!getline(csv, line)) { // Con getline() tomamos la próxima fila del archivo csv.
            break; // fin del archivo
        }

        // Extraer hasta 10 nombres de la fila
        vector<string> names = extract_summoner_names(line);

        // Agregar una entrada por jugador
        for (size_t i = 0; i < names.size(); ++i) {
            Entry e;
            e.hash16 = hash_reduce(names[i]);
            e.offset = (unsigned long long) row_off; 
            entries.push_back(e);
        }
        rows += 1;
    }

    // Creamos una función para comparar las entries ya que las queremos ordenar.
    struct LessEntry {
        bool operator()(const Entry& a, const Entry& b) const {
            if (a.hash16 < b.hash16) return true;
            if (a.hash16 > b.hash16) return false;
            return a.offset < b.offset;
        }
    };
    sort(entries.begin(), entries.end(), LessEntry());

    // Escribimos el archivo en out_path
    ofstream out(out_path, ios::binary | ios::trunc); // el trunc es para borrar si ya existe el archivo
    if (!out) {
        cerr << "No se pudo crear index_sorted.idx\n";
        return 1;
    }
    for (size_t i = 0; i < entries.size(); ++i) {
        const Entry& e = entries[i];
        out.write((const char*)&e.hash16, sizeof(e.hash16));  // 2 bytes
        out.write((const char*)&e.offset, sizeof(e.offset));  // 8 bytes
    }
    out.close();
    return 0;
}