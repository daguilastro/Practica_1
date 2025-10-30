# League of Legends Match Search System - v2.0

> **Práctica 1 - Sistemas Operativos**  
> Sistema cliente-servidor optimizado con búsqueda indexada, capacidad de añadir partidas y cierre controlado.

---

## Autores

- **[@daguilastro](https://github.com/daguilastro)** - Daniel Aguilar Castro
- **[@DeiberD](https://github.com/DeiberD)** - Deiber Gongora Hurtado
- **[@feliariasg](https://github.com/feliariasg)** - Felipe Arias G.

---

## Novedades de la Versión 2.0

### Nuevas Funcionalidades

1. **Comando `add`**: Añade nuevas partidas directamente desde el cliente
   - Ingresas los 10 nombres de jugadores
   - El sistema genera automáticamente una partida válida con stats realistas
   - Reconstruye el índice automáticamente

2. **Comando `exit`**: Cierra limpiamente todo el sistema
   - Detiene el servidor de forma controlada
   - Limpia recursos y sockets
   - Sin procesos huérfanos

---

## Cómo Usar el Sistema

### Inicio Rápido

```bash
# Compilar y ejecutar todo
make
```

```bash
# Compilar todo
make

# Iniciar el servidor (en una terminal)
./server_search

# En otra terminal, iniciar el cliente
./client_search
```

### Comandos Disponibles en el Cliente

```
 SISTEMA DE BÚSQUEDA DE PARTIDAS 
Comandos disponibles:
  - Escribe un nombre de jugador para buscar sus partidas
  - 'add' para añadir una nueva partida al dataset
  - 'exit' para salir del programa
__________________________________________________

>> Ingrese comando o nombre del jugador: _
```

---

## Ejemplos de Uso

### Buscar un Jugador

```bash
> Ingrese comando o nombre del jugador: Faker

[CLIENT] Conectando al servidor y enviando solicitud...

============================================================
Resultado: VICTORIA
Duración: 28:45
============================================================

EQUIPO AZUL (100):
  ► Faker
    DoinB
    Khan
    ...
```

### Añadir una Nueva Partida

```bash
> Ingrese comando o nombre del jugador: add

-   AÑADIR NUEVA PARTIDA   -
Por favor, ingresa los nombres de los 10 jugadores (separados por comas):
Formato: Jugador1,Jugador2,Jugador3,...,Jugador10
> MiNombre,Aliado1,Aliado2,Aliado3,Aliado4,Enemigo1,Enemigo2,Enemigo3,Enemigo4,Enemigo5

[CLIENT] Enviando solicitud para añadir partida...

[CLIENT] ¡Partida añadida exitosamente!
[CLIENT] El índice ha sido reconstruido
```

**El sistema generará automáticamente:**
- IDs únicos de partida
- Timestamp actual
- Estadísticas realistas (K/D/A, CS, Gold, Daño, etc.)
- División de equipos (5 vs 5)
- Resultado de la partida (Victoria/Derrota)

Ahora puedes buscar **cualquiera de esos 10 nombres** y verás la partida recién añadida.

### Salir del Sistema

```bash
> Ingrese comando o nombre del jugador: exit

[CLIENT] Enviando señal de cierre al servidor...
[CLIENT] Cerrando cliente. ¡Hasta luego!
```

El servidor también se cerrará automáticamente:
```
[SERVER] ¡Comando EXIT recibido!
[SERVER] Cerrando servidor limpiamente...
[SERVER] Realizando limpieza...
[SERVER] Servidor cerrado correctamente. ¡Adiós!
```

---

## Detalles Técnicos de las Nuevas Funciones

### Comando `add` - Flujo Interno

1. **Cliente detecta "add"** y solicita los 10 nombres
2. **Cliente envía**: `ADD_MATCH:Nombre1,Nombre2,...,Nombre10`
3. **Servidor recibe** el comando
4. **Servidor genera** una línea CSV completa con:
   - Match ID y Game ID únicos (generados aleatoriamente)
   - Timestamp actual en milisegundos
   - JSON de identidades con los 10 jugadores
   - JSON de participantes con estadísticas simuladas:
     - Kills/Deaths/Assists aleatorios pero realistas
     - CS (Creep Score) entre 50-250
     - Gold entre 3000-13000
     - Daño a campeones entre 5000-25000
     - Vision Score, nivel, items, etc.
   - División automática: primeros 5 = equipo azul (100), últimos 5 = equipo rojo (200)
5. **Servidor añade** la línea al `dataset.csv`
6. **Servidor ejecuta** `./build_sorted_index` para reconstruir el índice
7. **Servidor responde** `ADDED_OK` al cliente

---

## Ejemplo Completo de Sesión

```bash
$ ./server_search &
[SERVER] Socket creado en /tmp/demo_unix_epoll.sock
[SERVER] Epoll creado correctamente
[SERVER] Servidor listo, esperando conexiones...

$ ./client_search

-   SISTEMA DE BÚSQUEDA DE PARTIDAS   -
Comandos disponibles:
  - Escribe un nombre de jugador para buscar sus partidas
  - 'add' para añadir una nueva partida al dataset
  - 'exit' para salir del programa
_____________________________________________________

> Ingrese comando o nombre del jugador: add

-   AÑADIR NUEVA PARTIDA   -
Por favor, ingresa los nombres de los 10 jugadores (separados por comas):
Formato: Jugador1,Jugador2,...,Jugador10
> ProfeOmar,Estudiante1,Estudiante2,Estudiante3,Estudiante4,Rival1,Rival2,Rival3,Rival4,Rival5

[CLIENT] Enviando solicitud para añadir partida...

[CLIENT] ¡Partida añadida exitosamente!
[CLIENT] El índice ha sido reconstruido

>> Ingrese comando o nombre del jugador: ProfeOmar

[CLIENT] Conectando al servidor y enviando solicitud...

================= PARTIDA #1 =================

============================================================
Resultado: ✓ VICTORIA
Duración: 18:32
============================================================

EQUIPO AZUL (100):
  ► ProfeOmar
    Estudiante1
    Estudiante2
    Estudiante3
    Estudiante4

EQUIPO ROJO (200):
    Rival1
    Rival2
    Rival3
    Rival4
    Rival5

ESTADÍSTICAS DE ProfeOmar:
------------------------------------------------------------
K/D/A:             7/3/12 (KDA: 6.33)
CS:                143
Gold:              8956
Daño a campeones:  14523
Daño recibido:     7261
Vision Score:      34
Nivel final:       13
============================================================

Presiona ENTER para ver la siguiente partida (o 'q' + ENTER para salir):

> Ingrese comando o nombre del jugador: exit

[CLIENT] Enviando señal de cierre al servidor...
[CLIENT] Cerrando cliente. ¡Hasta luego!

[SERVER] ¡Comando EXIT recibido!
[SERVER] Cerrando servidor limpiamente...
[SERVER] Servidor cerrado correctamente. ¡Adiós!
```

---

## Compilación

### Compilación Manual
```bash
# Indexador
g++ -o build_sorted_index build_sorted_index.cpp

# Servidor
g++ -o server_search server_search.cpp func_server.cpp

# Cliente  
g++ -o client_search client_search.cpp func_client.cpp
```

---

## Archivos Modificados

### Nuevos/Modificados
- `client_search.cpp` - Añadido comando `add` 
- `server_search.cpp` - Manejo de comandos especiales
- `func_server.cpp` - Función `generateCSVLine()` para crear partidas
- `func_server.hpp` - Declaraciones actualizadas

### Sin Cambios
- `build_sorted_index.cpp`
- `process_matches.py`
- `func_client.cpp/hpp`
- Dataset CSV (se modifica dinámicamente con `add`)

---

## Notas Importantes

### Sobre el Comando `add`

1. **Requiere exactamente 10 nombres** separados por comas
2. **No uses caracteres especiales** en los nombres (solo letras, números, espacios)
3. **El índice se reconstruye automáticamente** - puede tomar unos segundos con datasets grandes
4. **Las estadísticas son simuladas** pero realistas
5. **El equipo azul siempre gana** en las partidas generadas (puedes modificar esto en `generateCSVLine()`)

---

## Troubleshooting

### Error: "Se requieren exactamente 10 jugadores"
```bash
# Solución: Ingresa 10 nombres separados por comas
> Nombre1,Nombre2,Nombre3,Nombre4,Nombre5,Nombre6,Nombre7,Nombre8,Nombre9,Nombre10
```

### Error: "No se pudo conectar al servidor"
```bash
# Verifica que el servidor esté corriendo
ps aux | grep server_search

# Si no está, inícialo
./server_search &
```

### El índice no se reconstruye
```bash
# Verifica que build_sorted_index esté compilado
ls -la build_sorted_index

# Si no existe, compílalo
g++ -o build_sorted_index build_sorted_index.cpp
```
