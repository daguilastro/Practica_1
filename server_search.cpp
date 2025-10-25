#include "func_server.hpp"

using namespace std;

int main() {
  // las direcciones de ambas pipes
  const char *request_pipe = "/tmp/search_request";
  const char *response_pipe = "/tmp/search_response";

  // Crear named pipes si no existen
  mkfifo(request_pipe,
         0666); // 0666 son permisos en octal aquí creamos como tal las pipes
  mkfifo(response_pipe, 0666);

  while (true) {
    char buffer[256]; // creamos un buffer para leer el nombre escrito
    recieveRequest(request_pipe, buffer);
    string name = string(buffer);

    // Verificar comando de salida
    if (name == "EXIT_SERVER") {
      cout << "[SERVER] Comando de salida recibido. Cerrando servidor...\n";
      break;
    }

    // Realizar búsqueda
    string result = searchServer(name);

    // Enviar resultado
    sendResult(response_pipe, result);
  }

  // Limpiar pipes
  unlink(request_pipe);
  unlink(response_pipe);

  return 0;
}