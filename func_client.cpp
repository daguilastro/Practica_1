#include "func_client.hpp"
#include <cstdio>

// Enviamos por la pipe /tmp/search_request el nombre que vamos a buscar
void send_request(const string& name){
    const char* request_pipe = "/tmp/search_request"; // Dirección donde esta guardada la pipe
    
    int fd = open(request_pipe, O_WRONLY);  // abrimos la pipe en modo Write Only.
    if(fd < 0){
        cerr << "[CLIENT] Error: No se pudo conectar al servidor.\n";
        cerr << "[CLIENT] Asegúrate de que server_search esté ejecutándose.\n";
        return;
    }
    
    write(fd, name.c_str(), name.size());   // Escribimos el nombre
    close(fd);  // Cerramos la pipe de nuestro lado
}

// Recibimos por la pipe /tmp/search_response los datos donde econtramos al jugador
string receive_response(){
    const char* response_pipe = "/tmp/search_response";
    int fd = open(response_pipe, O_RDONLY); // Creamos la pipe en modo Read Only
    if(fd < 0){
        return "[CLIENT] Error: No se pudo recibir respuesta del servidor.\n";
    }
    
    string result = "";
    char buffer[4096];  // Creamos un buffer donde iremos metiendo los datos enviados
    
    while(true){
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);  // Esperamos los datos de fd y los metemos al buffer
        
        if(bytes_read <= 0) break;  // Si acabó de leer todo terminamos.
        
        result += string(buffer, bytes_read);   // añadimos al resultado los datos guardados en el buffer
    }
    
    close(fd);
    return result;
}

// Mostramos de forma estética el resultado enviado del servidor
void display_matches_interactive(const string& all_matches) {
    
    size_t pos = 0;
    size_t next_pos;
    int match_num = 1;
    
    while (true) {
        // Buscar el próximo NEXT_MATCH
        next_pos = all_matches.find("NEXT_MATCH", pos);
        
        // Extraer la partida actual
        string match;
        if (next_pos != string::npos) {
            match = all_matches.substr(pos, next_pos - pos);
        } else {
            // Última partida (o única)
            match = all_matches.substr(pos);
        }
        
        cout << match << endl;
        
        // Si no hay más partidas, terminar
        if (next_pos == string::npos) {
            break;
        }
        
        // Preguntar si continuar
        cout << "\nPresiona ENTER para ver la siguiente partida (o 'q' + ENTER para salir): ";
        string input;
        getline(cin, input);
        
        if (!input.empty() && (input[0] == 'q' || input[0] == 'Q')) {
            cout << "Visualización cancelada.\n";
            return;
        }
        
        // Mover posición después de NEXT_MATCH
        pos = next_pos + 10;  // 10 = longitud de "NEXT_MATCH"
        
        // Saltar el newline después de NEXT_MATCH si existe
        if (pos < all_matches.length() && all_matches[pos] == '\n') {
            pos++;
        }
        
        match_num++;
    }
}