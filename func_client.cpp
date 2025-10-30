#include "func_client.hpp"
#include <cstdio>

// 
// UTILIDADES
// 
string trim(const string &s) {
	string result = s;

	// Eliminar espacios del final
	while (!result.empty() && (result.back() == ' ' || result.back() == '\t' || result.back() == '\r' || result.back() == '\n')) {
		result.pop_back();
	}

	// Eliminar espacios del inicio
	size_t i = 0;
	while (i < result.size() && (result[i] == ' ' || result[i] == '\t' || result[i] == '\r' || result[i] == '\n')) {
		++i;
	}

	return result.substr(i);
}

// 
// FUNCIONES DE COMUNICACIÓN
// 
string sendRequestAndReceive(const string &name) {
	// Crear socket UNIX
	int sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		cerr << "[CLIENT] Error creando socket\n";
		return "[CLIENT] Error de conexión\n";
	}

	// Configurar dirección del servidor
	struct sockaddr_un server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sun_family = AF_UNIX; // Familia de direcciones UNIX (local)
	strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

	// Conectar al servidor (es bloqueante, espera hasta conectar)
	if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		cerr << "[CLIENT] Error: No se pudo conectar al servidor\n";
		cerr << "[CLIENT] Asegúrate de que server_search esté ejecutándose\n";
		close(sock_fd);
		return "[CLIENT] Error de conexión\n";
	}

	cout << "[CLIENT] Conectado al servidor\n";

	// Enviar nombre del jugador (es IMPORTANTE agregar \n al final)
	// El servidor usa '\n' como delimitador para saber cuando terminó el mensaje
	string message = name + "\n";
	ssize_t bytes_sent = send(sock_fd, message.c_str(), message.size(), 0);
	if (bytes_sent < 0) {
		cerr << "[CLIENT] Error enviando solicitud\n";
		close(sock_fd);
		return "[CLIENT] Error enviando solicitud\n";
	}

	cout << "[CLIENT] Solicitud enviada, esperando respuesta...\n";

	// Recibir respuesta completa del servidor
	string response = "";
	char buffer[BUFFER_SIZE];
	size_t total_bytes = 0;

	// Leer en bucle hasta que el servidor cierre la conexión (bytes_recv == 0)
	while (true) {
		ssize_t bytes_recv = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
		cout << "CLIENTE RECIBE BYTES: " << bytes_recv << "\n";
		fflush(stdout);
		if (bytes_recv < 0) {
			// Error al recibir
			cerr << "[CLIENT] Error recibiendo respuesta\n";
			break;
		} else if (bytes_recv == 0) {
			// Servidor cerró la conexión - respuesta completa recibida
			break;
		}

		// Agregar datos recibidos a la respuesta
		response.append(buffer, bytes_recv);
		total_bytes += bytes_recv;
	}

	close(sock_fd);
	return response;
}

// 
// FUNCIONES DE DISPLAY
// 

void displayMatchesInteractive(const string &response) {
	// Verificar si no hay resultados
	if (response == "NA\n") {
		cout << "\nNo se encontraron resultados.\n";
		return;
	}

	// Verificar si hay error
	if (response.find("ERROR") == 0) {
		cout << "\n" << response;
		return;
	}

	size_t pos = 0;
	int match_num = 1;

	// Iterar sobre cada partida separada por "NEXT_MATCH"
	while (true) {
		// Buscar el próximo separador "NEXT_MATCH"
		size_t next_pos = response.find("NEXT_MATCH", pos);

		// Extraer la partida actual
		string match;
		if (next_pos != string::npos) {
			// Hay mas partidas después
			match = response.substr(pos, next_pos - pos);
		} else {
			// Última partida (o única)
			match = response.substr(pos);

			// Si está vacío, ya terminamos
			if (trim(match).empty()) {
				break;
			}
		}

		// Mostrar la partida con su número
		cout << "\n-          PARTIDA #" << match_num << "          -\n";
		cout << match << endl;

		// Si no hay más partidas, terminar
		if (next_pos == string::npos) {
			break;
		}

		// Preguntar al usuario si quiere continuar
		cout << "\nPresiona ENTER para ver la siguiente partida (o 'q' + ENTER para salir): ";
		string input;
		getline(cin, input);

		if (!input.empty() && (input[0] == 'q' || input[0] == 'Q')) {
			cout << "Visualización cancelada. Quedan " << ((response.size() - next_pos) / 1000) << " KB sin mostrar.\n";
			return;
		}

		// Mover posición después de "NEXT_MATCH"
		pos = next_pos + 10; // 10 = longitud de "NEXT_MATCH"

		// Saltar el salto de línea después de NEXT_MATCH si existe
		if (pos < response.length() && response[pos] == '\n') {
			pos++;
		}

		match_num++;
	}

	// Mostrar resumen final
	cout << "\n\n";
	cout << "Total de partidas mostradas: " << (match_num - 1) << "\n";
	cout << "__________________________________\n";
}