#pragma once

#include <chrono>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <signal.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

using namespace std;

// ============================================
// CONSTANTES
// ============================================
#define SOCKET_PATH "/tmp/demo_unix_epoll.sock"
#define MAX_EVENTS 30
#define BUFFER_SIZE 256

static const uint32_t HASH_MOD = 65536;

// ============================================
// ESTRUCTURAS
// ============================================
struct Entry {
    uint16_t hash16;
    uint64_t offset;
};

// ============================================
// FUNCIONES DE HASH
// ============================================
uint32_t fnv1a32(const string &s, uint32_t HASH_MOD);

// ============================================
// FUNCIONES DE BÚSQUEDA EN DATASET
// ============================================
bool read_csv_line_at(ifstream &csv, uint64_t off, string &out);
int64_t lower_bound_hash(ifstream &idx, uint16_t target, uint64_t N);
int64_t upper_bound_hash(ifstream &idx, uint16_t target, uint64_t N, uint64_t L);
string searchServer(const string &summoner_name);

// ============================================
// FUNCIONES DE SOCKET Y EPOLL
// ============================================
int createNBSocket();
void setNonBlocking(int fd);
int addSocketToEpoll(int fdEpoll, int fdSocket, uint32_t events);
int removeSocketFromEpoll(int fdEpoll, int fdSocket);

// ============================================
// FUNCIONES DE MANEJO DE CLIENTES
// ============================================
int acceptNewClient(int fdServer, int fdEpoll);
void handleClientData(int clientFd, int fdEpoll);
string receiveFromClient(int clientFd);
bool sendToClient(int clientFd, const string &data);
void cleanupClient(int clientFd);

// ============================================
// UTILIDADES
// ============================================
string trim(const string &s);