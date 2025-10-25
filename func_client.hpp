#pragma once
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>


using namespace std;

struct RowInfo {
    // Resultado
    int resultado;              // 1 = ganó, 0 = perdió
    double duracion;            // minutos
    
    // Equipos
    vector<string> equipo_azul;
    vector<string> equipo_rojo;
    
    // Stats del jugador
    int kills;
    int deaths;
    int assists;
    int cs;
    int gold_earned;
    int damage_to_champions;
    int damage_taken;
    int vision_score;
    int level;
    double kda;
};

void send_request(const string& name);

string receive_response();

void display_matches_interactive(const string& all_matches);