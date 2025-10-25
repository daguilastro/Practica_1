#pragma once
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <cstdio>


using namespace std;

void send_request(const string& name);

string receive_response();

void display_matches_interactive(const string& all_matches);