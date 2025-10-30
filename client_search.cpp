#include "func_client.hpp"

int main() {
    cout << "\nBusqueda De Partidas\n";
    cout << "Comandos disponibles:\n";
    cout << "- Escribe un nombre de jugador para buscar sus partidas\n";
    cout << "- 'add' para añadir una nueva partida al dataset\n";
    cout << "- 'exit' para salir del programa\n";
    cout << "_________________________________________________________________\n\n";

    // Bucle principal del cliente
    while (true) {
        cout << ">> Ingrese comando o nombre del jugador: ";

        string input;
        getline(cin, input);
        input = trim(input);

        if (input.empty()) continue;

        // Comando EXIT para acabar con el programa y salir
        if (input == "exit" || input == "EXIT") {
            cout << "\n[CLIENT] Enviando señal de cierre al servidor...\n";
            sendRequestAndReceive("EXIT");
            cout << "[CLIENT] Cerrando cliente...\n";
            break;
        }

        // Comando ADD para añadir nueva partida al dataset
        if (input == "add" || input == "ADD") {
            cout << "\n - AÑADIR NUEVA PARTIDA -\n";
            cout << "Ingresa los nombres de los 10 jugadores (separados por comas):\n";
            cout << "Formato: Jugador1, Jugador2, Jugador3, Jugador4, Jugador5, Jugador6, Jugador7, Jugador8, Jugador9, Jugador10\n";
            cout << ">> ";
            
            string players_input;
            getline(cin, players_input);
            players_input = trim(players_input);
            
            if (players_input.empty()) {
                cout << "[CLIENT] Operación cancelada - no se ingresaron nombres\n\n";
                continue;
            }

            // Aqui se envia el comando al servidor para añadir una partida
            cout << "\n[CLIENT] Enviando solicitud para añadir partida...\n";
            string add_command = "ADD_MATCH:" + players_input;
            string response = sendRequestAndReceive(add_command);
            
            // Mostrar la respuesta del servidor
            if (response.find("ADDED_OK") != string::npos) {
                cout << "\n[CLIENT] Partida añadida exitosamente\n";
                cout << "[CLIENT] El indice ya se ha reconstruido\n\n";
            } else if (response.find("ERROR") != string::npos) {
                cout << "\n[CLIENT] Error al añadir partida:\n";
                cout << response << "\n";
            } else {
                cout << "\n[CLIENT] Respuesta del servidor:\n" << response << "\n";
            }
            continue;
        }

        // Busqueda normal de jugador
        cout << "\n[CLIENT] Conectando al servidor y enviando solicitud...\n";
        string response = sendRequestAndReceive(input);

        // Mostrar resultados
        displayMatchesInteractive(response);
        cout << "\n";
    }

    return 0;
}