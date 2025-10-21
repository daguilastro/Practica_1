#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

using namespace std;

// Funcionalidad de string

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

void display_results(const string& response){
    if(response == "NA\n"){
        cout << "\nNo se encontraron resultados.\n";
        return;
    }
    
    if(response.find("ERROR") != string::npos){
        cout << response;
        return;
    }
    
    size_t pos = 0;
    
    while(true){
        size_t next_match = response.find("NEXT_MATCH", pos);
        if(next_match == string::npos) break;
        
        string match_info = response.substr(pos, next_match - pos);
        cout << match_info;
        
        cout << "Enter = siguiente,  q = salir > " << flush;
        string cmd;
        getline(cin, cmd);
        
        if(!cmd.empty() && (cmd[0] == 'q' || cmd[0] == 'Q')){
            cout << "\nBúsqueda interrumpida por el usuario.\n";
            return;
        }
        
        pos = next_match + 10;  // salta el texto "next_match" ya que usamos este texto como separador de las partidas
    }
    
    cout << "\nNo hay más resultados.\n";
}

int main(){

    while(true){
        cout << "Ingrese el nombre del jugador a buscar (o 'exit' para salir): ";
        
        string name;
        getline(cin, name);     // Escribimos el nombre a buscar
        name = trim_copy(name);
        
        if(name.empty()) continue;
        
        if(name == "exit" || name == "EXIT"){
            cout << "\nCerrando cliente y servidor...\n";
            // Enviar señal de salida al servidor
            send_request("EXIT_SERVER");
            sleep(1);  // Dar tiempo al servidor para cerrar
            break;
        }
        
        cout << "\n[CLIENT] Enviando solicitud al servidor...\n";   // Le hacemos la petición al server
        send_request(name);
        
        cout << "[CLIENT] Esperando respuesta del servidor...\n";   // El servidor responde
        string response = receive_response();
        
        display_results(response);  // Mostramos la respuesta
        
        cout << "\n";   // repetimos el proceso
    }
    
    cout << "¡Hasta luego!\n";
    return 0;
}