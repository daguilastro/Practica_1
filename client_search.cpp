#include "func_client.hpp"

int main() {
    // Bucle principal del cliente
    while (true) {
        // Solicitar nombre del jugador
        cout << "Ingrese el nombre del jugador a buscar (o 'exit' para salir): ";
        
        string name;
        getline(cin, name);  // Leer línea completa (permite espacios)
        
        // Limpiar espacios en blanco
        name = trim(name);
        
        // Ignorar entradas vacías
        if (name.empty()) continue;
        
        // Comando para salir
        if (name == "exit" || name == "EXIT") {
            cout << "\nCerrando cliente y servidor...\n";
            // Enviar señal de salida al servidor
            sendRequestAndReceive("EXIT_SERVER");
            sleep(1);  // Dar tiempo al servidor para cerrar
            break;
        }
        
        cout << "\n[CLIENT] Conectando al servidor...\n";
        
        // Enviar solicitud y recibir respuesta del servidor
        string response = sendRequestAndReceive(name);
        
        // Mostrar resultados de forma interactiva (una partida a la vez)
        displayMatchesInteractive(response);
        
        cout << "\n";  // Separador entre búsquedas
    }
    
    cout << "¡Hasta luego!\n";
    return 0;
}