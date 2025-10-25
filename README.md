# 🎮 League of Legends Match Search System

> **Práctica 1 - Sistemas Operativos**  
> Un sistema cliente-servidor optimizado de búsqueda indexada de partidas de League of Legends, implementado en C++ con comunicación IPC mediante Named Pipes.

---

## 📋 Tabla de Contenidos

- [¿Qué es este proyecto?](#qué-es-este-proyecto)
- [Características Principales](#características-principales)
- [Requisitos](#requisitos)
- [Instalación y Compilación](#instalación-y-compilación)
- [Cómo Usar](#cómo-usar)
- [Arquitectura del Sistema](#arquitectura-del-sistema)
- [Componentes Técnicos](#componentes-técnicos)
- [Explicación de Algoritmos](#explicación-de-algoritmos)
- [Optimizaciones Implementadas](#optimizaciones-implementadas)
- [Estructura de Archivos](#estructura-de-archivos)
- [Troubleshooting](#troubleshooting)
- [Autores](#autores)

---

## ❓ ¿Qué es este proyecto?

Un **sistema de búsqueda de partidas de League of Legends** con arquitectura cliente-servidor que funciona localmente. Permite buscar jugadores por su nombre y ver todas sus partidas registradas con estadísticas detalladas.

**¿Cómo funciona en simple?**
1. Construye un **índice optimizado** de todas las partidas del dataset
2. El **servidor** recibe solicitudes de búsqueda por pipe
3. Busca usando **hash + binary search** (muy rápido)
4. Procesa los datos con **Python** para formatar
5. El **cliente** muestra los resultados de forma interactiva

---

## ✨ Características Principales

- ✅ **Búsqueda ultra-rápida** usando indexación con hash y binary search
- ✅ **Interfaz Cliente-Servidor** con comunicación por Named Pipes (IPC)
- ✅ **Visualización interactiva** de partidas (una a una, muy legible)
- ✅ **Estadísticas detalladas** de cada jugador:
  - K/D/A (Kills/Deaths/Assists)
  - CS (Creep Score)
  - Gold ganado
  - Daño infligido y recibido
  - Vision Score
  - Resultado de la partida (Victoria/Derrota)
  - Equipos (Azul vs Rojo)
- ✅ **Procesamiento en Python** para formateo de datos JSON
- ✅ **Compilación sencilla** con Makefile
- ✅ **Múltiples búsquedas** sin necesidad de reiniciar

---

## 🔧 Requisitos

### Sistema Operativo
- **Linux** (indispensable para Named Pipes y syscalls)
- Cualquier distribución (Ubuntu, Debian, Fedora, etc.)

### Software Necesario
```
g++ (GCC 9.0 o superior)
python3 (versión 3.6+)
make
pip (gestor de paquetes de Python)
```

### Instalación de Dependencias

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential g++ python3 python3-pip make
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ python3 python3-pip make
```

**Librería Python requerida:**
```bash
pip install ujson
```

### Archivo de Datos
- **`dataset.csv`** - Base de datos de partidas (debe estar en el directorio raíz)
- Formato: CSV con datos de partidas de League of Legends

---

## 📦 Instalación y Compilación

### Paso 1: Clonar el repositorio
```bash
git clone https://github.com/daguilastro/Practica_1.git
cd Practica_1
```

### Paso 2: Instalar dependencias Python
```bash
pip install ujson
```

### Paso 3: Compilar todo (RECOMENDADO)
```bash
make
```

Este comando hará **todo automáticamente**:
- Compila todos los programas
- Construye el índice
- Inicia el servidor
- Lanza el cliente interactivo
- Limpia archivos al terminar

### Paso 4 (Opcional): Compilación Manual

Si prefieres compilar manualmente:

```bash
# Solo el indexador
g++ -o build_sorted_index build_sorted_index.cpp

# Solo el servidor
g++ -o server_search server_search.cpp func_server.cpp

# Solo el cliente
g++ -o client_search client_search.cpp func_client.cpp
```

---

## 🚀 Cómo Usar

### Forma más fácil (Automática)
```bash
make
```
¡Y listo! Se ejecutará todo de forma automática.

---

### Forma manual paso a paso

#### **1️⃣ Crear el índice**
```bash
./build_sorted_index
```
Esto genera `index_sorted.idx` a partir del `dataset.csv`. Solo necesita ejecutarse una vez.

```
📁 index_sorted.idx (creado)
   └─ Archivo binario con hash y offsets de todas las partidas
```

#### **2️⃣ Iniciar el servidor**
```bash
./server_search &
```
El servidor se ejecuta en background y espera solicitudes por pipe.

```
[SERVER] Esperando conexiones en /tmp/search_request...
```

#### **3️⃣ Ejecutar el cliente**
```bash
./client_search
```

#### **4️⃣ Buscar jugadores**

```
Ingrese el nombre del jugador a buscar (o 'exit' para salir): Faker
[CLIENT] Enviando solicitud al servidor...
```

---

### Ejemplo completo de uso

```
$ make

g++ -o build_sorted_index build_sorted_index.cpp
g++ -o server_search server_search.cpp func_server.cpp
g++ -o client_search client_search.cpp func_client.cpp
./build_sorted_index
[BUILD] Construyendo índice...
[BUILD] ✅ Índice creado: index_sorted.idx
./server_search &
[SERVER] ✅ Servidor iniciado
./client_search

Ingrese el nombre del jugador a buscar (o 'exit' para salir): Faker

[CLIENT] Enviando solicitud al servidor...

============================================================
Resultado: ✓ VICTORIA
Duración: 28:45
============================================================

EQUIPO AZUL (100):
  ► Faker
    DoinB
    Khan
    Teddy
    CoreJJ

EQUIPO ROJO (200):
    ShowMaker
    Canyon
    Nuguri
    Ghost
    BuFu

ESTADÍSTICAS DE Faker:
------------------------------------------------------------
K/D/A:             8/2/15 (KDA: 11.50)
CS:                312
Gold:              14500
Daño a campeones:  18234
Daño recibido:     8921
Vision Score:      45.2
Nivel final:       18
============================================================

Presiona ENTER para ver la siguiente partida (o 'q' + ENTER para salir):
```

#### **Navegar resultados:**
- `ENTER` → Ver siguiente partida
- `q + ENTER` → Salir de la visualización
- `exit` → Cerrar cliente y servidor

---

## 🏗️ Arquitectura del Sistema

```
┌──────────────────────┐
│   CLIENTE            │
│ (client_search)      │
│                      │
│ • Input del usuario  │
│ • Envía búsqueda     │
│ • Muestra resultados │
└──────────┬───────────┘
           │
           │ /tmp/search_request
           │ (Named Pipe - FIFO)
           │
           ▼
┌──────────────────────────────────────┐
│         SERVIDOR                     │
│      (server_search)                 │
│                                      │
│  ┌──────────────────────────────┐   │
│  │ 1. Recibe solicitud          │   │
│  │ 2. Calcula hash FNV-1a       │   │
│  │ 3. Búsqueda binaria en idx   │   │
│  │ 4. Lee datos del CSV         │   │
│  └──────────────────────────────┘   │
│                ▼                     │
│  ┌──────────────────────────────┐   │
│  │ FORK: Ejecuta Python         │   │
│  │ • Procesa JSON               │   │
│  │ • Extrae estadísticas        │   │
│  │ • Formatea resultados        │   │
│  └──────────────────────────────┘   │
└──────────┬───────────────────────────┘
           │
           │ /tmp/search_response
           │ (Named Pipe - FIFO)
           │
           ▼
┌──────────────────────┐
│   CLIENTE            │
│ (recibe resultado)   │
│ (muestra en pantalla)│
└──────────────────────┘
```

---

## 🔌 Componentes Técnicos

### 1. **build_sorted_index.cpp** - Constructor del Índice 🗂️

**¿Qué hace?**
Lee el CSV completo, extrae nombres de jugadores y crea un índice binario ordenado.

**Proceso:**
```
dataset.csv (millones de registros)
    ↓
Leer cada fila
    ↓
Extraer 10 nombres de jugadores por fila
    ↓
Calcular hash FNV-1a de cada nombre
    ↓
Reducir hash: hash % 65536
    ↓
Ordenar por hash (radix/merge sort)
    ↓
Escribir a index_sorted.idx (formato binario)
    └─ 10 bytes por entrada: 2 bytes (hash) + 8 bytes (offset)
```

**Complejidad:** O(n log n)

### 2. **server_search.cpp** - Servidor Principal 🖥️

**Funciones clave:**

```cpp
// Busca la primera ocurrencia del hash
int64_t lower_bound_hash(ifstream &idx, uint16_t target, uint64_t N)

// Busca la última ocurrencia del hash
int64_t upper_bound_hash(ifstream &idx, uint16_t target, uint64_t N, uint64_t L)

// Búsqueda completa
string searchServer(string summoner_name)

// Comunicación con cliente
void recieveRequest(const char *request_pipe, char *buffer)
void sendResult(const char *response_pipe, string &result)
```

**¿Qué hace?**
1. Crea dos Named Pipes (request y response)
2. Espera solicitudes del cliente
3. Busca en el índice usando binary search
4. Lee datos del CSV
5. Fork de proceso Python para procesar
6. Envía resultado al cliente

**Complejidad:** O(log n) para la búsqueda

### 3. **client_search.cpp** - Cliente Interactivo 💻

**Funciones clave:**

```cpp
// Envía nombre al servidor por pipe
void send_request(const string& name)

// Recibe respuesta del servidor
string receive_response()

// Muestra resultados de forma interactiva
void display_matches_interactive(const string& all_matches)
```

**¿Qué hace?**
1. Loop infinito de búsquedas
2. Lee entrada del usuario
3. Envía por Named Pipe al servidor
4. Recibe respuesta
5. Muestra partidas una por una
6. Permite navegar con ENTER

### 4. **process_matches.py** - Procesador de Datos 🐍

**¿Qué hace?**
1. Recibe datos CSV por pipe (file descriptor)
2. Parsea líneas del CSV
3. Convierte strings a JSON
4. Extrae identidades y participantes
5. Busca al jugador específico
6. Calcula estadísticas (KDA, CS, Gold, etc.)
7. Formatea salida bonita
8. Envía resultado por pipe

**¿Por qué Python?** Porque parsear JSON complicado en C++ es tedioso. Python es más limpio.

---

## 🧠 Explicación de Algoritmos

### Hash Function: FNV-1a 32-bit

```cpp
uint32_t fnv1a32(const string &s, uint32_t HASH_MOD) {
  const uint32_t FNV_PRIME = 16777619u;        // Número primo especial
  const uint32_t FNV_OFFSET = 2166136261u;     // Offset inicial

  uint32_t hash = FNV_OFFSET;
  for (char c : s) {
    hash ^= (uint8_t)c;           // XOR con cada carácter
    hash *= FNV_PRIME;            // Multiplica por primo
  }

  return hash % HASH_MOD;         // Reduce a rango [0, 65535]
}
```

**¿Por qué FNV-1a?**
- ✅ Rápida (una operación por carácter)
- ✅ Distribución uniforme
- ✅ Baja tasa de colisiones
- ✅ Muy usada en sistemas

**Ejemplo:**
```
"Faker" → hash = FNV_OFFSET
"Faker"[0] = 'F' → hash = (hash ^ 'F') * FNV_PRIME
"Faker"[1] = 'a' → hash = (hash ^ 'a') * FNV_PRIME
...
final hash = hash % 65536
```

### Binary Search: Lower/Upper Bound

**Lower Bound** (encuentra el primero):
```
Target: hash = 1000
Index:  [500, 700, 1000, 1000, 1000, 1500, 2000]
         ├────────┤     └─ LOWER BOUND
                        ↑ Primer 1000

Devuelve el índice 2
```

**Upper Bound** (encuentra después del último):
```
Target: hash = 1000
Index:  [500, 700, 1000, 1000, 1000, 1500, 2000]
                          └─ UPPER BOUND
                                   ↑ Primer elemento > 1000

Devuelve el índice 5
```

**Uso:**
```cpp
int64_t L = lower_bound_hash(idx, hash, N);   // Primera ocurrencia
int64_t R = upper_bound_hash(idx, hash, N, L);  // Primera NO-ocurrencia

// Rango: [L, R) contiene todas las partidas del jugador
for (int64_t i = L; i < R; i++) {
    // Procesar partida i
}
```

**Complejidad:** O(log n)

### Named Pipes (FIFO)

```
// Crear pipes
mkfifo("/tmp/search_request", 0666);
mkfifo("/tmp/search_response", 0666);

// Cliente escribe solicitud
int fd = open("/tmp/search_request", O_WRONLY);
write(fd, "Faker", 5);
close(fd);

// Servidor lee
int fd = open("/tmp/search_request", O_RDONLY);
char buffer[256];
read(fd, buffer, sizeof(buffer));
close(fd);

// Servidor escribe respuesta
int fd = open("/tmp/search_response", O_WRONLY);
write(fd, results, result_size);
close(fd);

// Cliente lee respuesta
int fd = open("/tmp/search_response", O_RDONLY);
read(fd, buffer, sizeof(buffer));
close(fd);
```

**Ventajas:**
- ✅ IPC (Inter-Process Communication)
- ✅ Sincronización automática
- ✅ No necesita puertos de red
- ✅ Eficiente

### Fork + Exec para Python

```cpp
pid_t pid = fork();

if (pid == 0) {
    // PROCESO HIJO
    close(pipe_to_python[1]);
    close(pipe_from_python[0]);

    // Configura variables de entorno con file descriptors
    setenv("INPUT_FD", to_string(pipe_to_python[0]).c_str(), 1);
    setenv("OUTPUT_FD", to_string(pipe_from_python[1]).c_str(), 1);

    // Reemplaza este proceso con Python
    execlp("python3", "python3", "process_matches.py", summoner_name, nullptr);
    exit(1);
}

// PROCESO PADRE
close(pipe_to_python[0]);
close(pipe_from_python[1]);

// Envía datos
write(pipe_to_python[1], csv_data, size);
close(pipe_to_python[1]);

// Recibe resultado
read(pipe_from_python[0], buffer, sizeof(buffer));

// Espera a que termine
waitpid(pid, &status, 0);
```

---

## ⚡ Optimizaciones Implementadas

### 1. Indexación: O(n) → O(log n)

```
Sin indexación:
dataset.csv (1 millón de líneas)
    └─ Búsqueda lineal de "Faker"
    └─ Leo 1 millón de líneas
    └─ Complejidad: O(n)

Con indexación:
index_sorted.idx (ordenado por hash)
    └─ Binary search del hash
    └─ Leo solo ~log(n) entradas del índice
    └─ Complejidad: O(log n)

Mejora: 1,000,000 líneas → 20 comparaciones
```

### 2. Formato Binario del Índice

```
Struct Entry compacta:
├─ hash16:  2 bytes (uint16_t)
└─ offset:  8 bytes (uint64_t)
└─ TOTAL: 10 bytes por entrada

Archivo: 1 millón de jugadores × 10 bytes = 10 MB
Acceso directo sin parsear texto
```

### 3. Buffers Ampliados en Pipes

```cpp
int buffer_size = 1048576;  // 1 MB
fcntl(pipe_to_python[1], F_SETPIPE_SZ, buffer_size);
fcntl(pipe_from_python[1], F_SETPIPE_SZ, buffer_size);
```

Sin esto, los pipes están limitados a 64 KB y se bloquean.

### 4. Visualización Paginada

Cliente muestra partida a partida:
- Evita llenar la pantalla
- Mejor experiencia de usuario
- Manejo de muchos resultados

### 5. Paralelismo

```
Mientras el cliente visualiza resultado 1
El servidor ya está procesando solicitud 2
Máxima eficiencia
```

---

## 📊 Complejidad Algorítmica

| Operación | Complejidad | Tiempo Estimado (1M registros) |
|-----------|------------|-------------------------------|
| Construir Índice | O(n log n) | ~5 segundos |
| Búsqueda en índice | O(log n) | ~0.02 ms |
| Lectura CSV | O(k) | ~k ms (k = matches) |
| Procesamiento Python | O(k) | ~k ms |
| **Total** | **O(log n + k)** | **Sub-milisegundos** |

---

## 📝 Estructura de Archivos

```
Practica_1/
├── README.md                    ← Este archivo
├── Makefile                     ← Automatización compilación
│
├── build_sorted_index.cpp       ← Constructor de índice
│
├── server_search.cpp            ← Punto de entrada del servidor
├── func_server.cpp              ← Lógica del servidor
├── func_server.hpp              ← Headers del servidor
│
├── client_search.cpp            ← Punto de entrada del cliente
├── func_client.cpp              ← Lógica del cliente
├── func_client.hpp              ← Headers del cliente
│
├── process_matches.py           ← Procesador de datos (Python)
│
└── dataset.csv                  ← Base de datos (NO incluida)

Archivos generados en runtime:
├── build_sorted_index           ← Ejecutable
├── server_search                ← Ejecutable
├── client_search                ← Ejecutable
├── index_sorted.idx             ← Índice binario (generado)
└── /tmp/search_*                ← Named pipes
```

---

## 🐛 Troubleshooting

### ❌ Error: "No se pudo conectar al servidor"

```bash
# Verifica si el servidor está corriendo
ps aux | grep server_search

# Si no está, inicialo:
./server_search &

# Si tienes problemas con pipes viejas:
rm -f /tmp/search_request /tmp/search_response
./server_search &
```

### ❌ Error: "No pude abrir dataset.csv"

```bash
# Verifica que existe
ls -la dataset.csv

# Debe estar en el MISMO directorio donde ejecutas
pwd
# Debería estar en Practica_1/
```

### ❌ Error: "Python no encontrado"

```bash
# Instala Python3
sudo apt install python3

# Verifica
python3 --version

# Instala ujson
pip install ujson

# Verifica
python3 -c "import ujson; print('OK')"
```

### ❌ Error: "ModuleNotFoundError: No module named 'ujson'"

```bash
pip install ujson

# O específicamente para python3
pip3 install ujson
```

### ❌ Pipes no se limpian correctamente

```bash
# Limpia manualmente
rm -f /tmp/search_request /tmp/search_response

# O mata todos los procesos relacionados
pkill -f server_search
pkill -f client_search
```

### ❌ Compilación falla

```bash
# Asegúrate de tener g++
g++ --version

# Si no:
sudo apt install build-essential

# Limpia y recompila
make clean
make
```

---

## 👨‍💻 Autores

Este proyecto fue desarrollado por:

- **[@daguilastro](https://github.com/daguilastro)** - David Aguilar - Desarrollo principal
- **[@DeiberD](https://github.com/DeiberD)** - Deiberzon - Colaborador
- **[@feliariasg](https://github.com/feliariasg)** - Felicias - Colaborador

---

## 📄 Licencia

Este proyecto está disponible bajo licencia **MIT**.

---

## 📌 Notas Finales

- 🔒 **Seguridad**: Las pipes usan permisos 0666 (cualquiera puede acceder)
- ⚡ **Rendimiento**: Diseñado para datasets grandes
- 🐧 **Plataforma**: Solo Linux (usa syscalls POSIX)
- 🔄 **Reutilizable**: Arquitectura modular y limpia
- 📚 **Educativo**: Excelente para aprender Sistemas Operativos

---

**¡Gracias por usar este proyecto! ⭐**

Si te resulta útil, considera darle una estrella en GitHub.
