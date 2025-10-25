#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <chrono>

using namespace std;
static uint32_t HASH_MOD = 65536;

// Función hash reducida
uint32_t fnv1a32(const string &s, uint32_t HASH_MOD);

// La struct de como están ordenados los archivos en la tabla hash
struct Entry {
  uint16_t hash16;
  uint64_t offset;
};

// Guarda en out todo la fila del csv al que se apuntó
bool read_csv_line_at(ifstream &csv, uint64_t off, string &out);

int64_t lower_bound_hash(ifstream &idx, uint16_t target, uint64_t N);

// búsqueda binaria para el limite superior
int64_t upper_bound_hash(ifstream &idx, uint16_t target, uint64_t N,
                         uint64_t L);

string searchServer(string summoner_name);

void recieveRequest(const char *request_pipe, char *buffer);

void sendResult(const char *response_pipe, string &result);