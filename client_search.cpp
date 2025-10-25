#include "func_client.hpp"
#include <cstdio>

int main(){
    while(true){
        cout << "Ingrese el nombre del jugador a buscar (o 'exit' para salir): ";
        
        string name;
        getline(cin, name);     // Escribimos el nombre a buscar
        
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
        string response = receive_response();
        display_matches_interactive(response);
    }
    
    cout << "¡Hasta luego!\n";
    return 0;
}