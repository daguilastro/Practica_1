#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace std;

// ============================================
// CONSTANTES
// ============================================
#define SOCKET_PATH "/tmp/demo_unix_epoll.sock"  // Ruta del socket UNIX para comunicación
#define BUFFER_SIZE 4096  // Tamaño del buffer de lectura (4KB)

// ============================================
// FUNCIONES DE COMUNICACIÓN
// ============================================

// Conecta al servidor, envía el nombre del jugador y recibe la respuesta completa
// Parámetro: name - nombre del jugador a buscar
// Retorna: string con todas las partidas encontradas o mensaje de error
string sendRequestAndReceive(const string& name);

// ============================================
// FUNCIONES DE DISPLAY
// ============================================

// Muestra las partidas de forma interactiva (una por una, esperando ENTER)
// Parámetro: response - string completo con todas las partidas separadas por "NEXT_MATCH"
void displayMatchesInteractive(const string& response);

// ============================================
// UTILIDADES
// ============================================

// Elimina espacios, tabs y saltos de línea al inicio y final de un string
// Parámetro: s - string a limpiar
// Retorna: string sin espacios en los extremos
string trim(const string &s);