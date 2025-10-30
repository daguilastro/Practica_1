#include "func_client.hpp"

int main() {
    // Bucle principal del cliente
    while (true) {
        cout << "Ingrese el nombre del jugador a buscar: ";

        string name;
        getline(cin, name);  // Leer línea completa (permite espacios)
        name = trim(name);

        if (name.empty()) continue;
        cout << "el nombre que se va a mandar a buscaer " << name << "\n";
        cout << "\n[CLIENT] Conectando al servidor y enviando solicitud...\n";
        string response = sendRequestAndReceive(name);

        // Mostrar resultados
        displayMatchesInteractive(response);
        cout << "\n"; // separador entre búsquedas
    }

    return 0;
}