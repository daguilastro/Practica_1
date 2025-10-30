#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

using namespace std;

// 
// CONSTANTES
// 
#define SOCKET_PATH "/tmp/demo_unix_epoll.sock"  // Ruta del socket UNIX para comunicación
#define BUFFER_SIZE 4096  // Tamaño del buffer de lectura (4KB)


// 
// FUNCIONES DE COMUNICACIÓN
// 

// Conecta al servidor, envía el nombre del jugador y recibe la respuesta completa
// Parámetro: name - nombre del jugador a buscar (no se le añade '\n' porque la función lo añade)
// Retorna: string con todas las partidas encontradas o mensaje de error
string sendRequestAndReceive(const string& name);


// 
// FUNCIONES DE DISPLAY
// 

// Muestra las partidas de forma interactiva, por decirlo asi (una por una, esperando ENTER)
// Parámetro: response - string completo con todas las partidas separadas por "NEXT_MATCH"
void displayMatchesInteractive(const string& response);


// 
// UTILIDADES
// 

// Elimina espacios, tabs y saltos de línea al inicio y final de un string
string trim(const string &s);