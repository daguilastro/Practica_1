# Práctica 1 - Sistemas Operativos

Procesos y comunicación entre procesos.</br>

## Integrantes

### Daniel Aguilar Castro - daaguilarc@unal.edu.co
### Andres Felipe Arias Gonzalez - anariasg@unal.edu.co
### Deiber David Gongora Hurtado - dgongora@unal.edu.co </br>

## Descripción de la práctica

Usamos un dataset de 1Gb que guarda los datos de todas las partidas que se jugaron en 2020 en challenger korea para ejecutar una bùsqueda en menos de dos segundos usando como criterio el nombre de un jugador. Tècnicamente usamos dos procesos no emparentados que se comunican a travès de named pipes (una para enviar el criterio y otra para enviar la respuesta) La bùsqueda se hace usando una tabla hash indexada y ordenada, encontramos la posiciòn del hash usando bùsqueda binaria para posteriormente irnos directamente a la posiciòn de los datos en la csv. </br>


## Campos utilizados:

### summonerName (string):

Nombre del invocador/jugador</br>
Se encuentra dentro del JSON participantIdentities</br>
Formato en CSV: 'summonerName': 'NombreJugador'</br>
Hay 10 nombres por fila (10 jugadores por partida)</br>
Propósito: Campo principal de búsqueda, se indexa con hash FNV-1a</br>


### gameDuration (float):

Duración de la partida en segundos</br>
Segunda columna del CSV (después del índice)</br>
Propósito: Se muestra en minutos:segundos en los resultados</br>


### participantId (int):

ID del jugador dentro de la partida (1-10)</br>
Se encuentra en participantIdentities</br>
Propósito: Relacionar el jugador con sus estadísticas</br>


### win (boolean):

Indica si el jugador ganó o perdió</br>
Se encuentra en participants[i].stats.win</br>
Valores: True o False</br>
Propósito: Determinar el resultado de la partida para ese jugador</br>

## Implementación De Búsqueda Exacta


### Técnica de indexación: 
Hash Table con búsqueda binaria</br>

### Decisiones de diseño:
Hash FNV-1a de 32 bits reducido a 16 bits:</br>

```cpp
if (line.find(name) == string::npos) continue;
```

### Uso de seekg() para acceso aleatorio:
No carga el dataset completo en memoria </br>
Solo lee las líneas necesarias usando offsets guardados</br>


## Rangos de Valores Válidos

### Hash
hash16:           [0, 65535]        // uint16_t, módulo 65536</br>

### Offset en CSV  
offset:           [0, 2^64-1]       // uint64_t, posición en bytes</br>

### Datos extraídos
gameDuration:     [180.0, 7200.0]   // 3 min - 2 horas (estimado)</br>
participantId:    [1, 10]           // Siempre 10 jugadores</br>
win:              {True, False}     // Boolean</br>

### summonerName
longitud:         [3, 16]           // Según límites de Riot Games</br>
caracteres:       UTF-8             // Incluye coreano, chino, etc.</br>


## Ejemplos Específicos de Uso del Programa

### Compilación:

```bash
make 
```

Esto genera:</br>
build_sorted_index → Construye el índice</br>
server_search → Servidor de búsqueda</br>
client_search → Cliente interactivo</br>

### Ejecución completa:

```bash
make run
```

Esto:
Construye el índice (index_sorted.idx)</br>
Inicia el servidor en background</br>
Ejecuta el cliente</br>
Cierra todo automáticamente</br>

## Ejemplo 1: Búsqueda básica

```bash
$ ./client_search
```

Ingrese el nombre del jugador a buscar (o 'exit' para salir): DWG Canyon
</br>
[CLIENT] Enviando solicitud al servidor...</br>
[CLIENT] Esperando respuesta del servidor...</br>
</br>
================= PARTIDA =================</br>
Jugador buscado : DWG Canyon</br>
Duracion        : 22 min 03 s</br>
Resultado       : Ganó</br>
Participantes   :</br>
  1. 쪼렙이다말로하자
  2. 불질러
  3. Youtube Thal
  4. 213321123
  5. ggffggg
  6. 곽김보태성민광희
  7. Gen G Clid
  8. Gen G Rascal
  9. 송아지얼룩송아지
  10. DWG Canyon
(offset CSV: 1234567)</br>
===========================================</br>
Enter = siguiente,  q = salir > </br>


### Ejemplo 2: Jugador no encontrado

Ingrese el nombre del jugador a buscar (o 'exit' para salir): FakerXD123
</br>
[CLIENT] Enviando solicitud al servidor...</br>
[CLIENT] Esperando respuesta del servidor...</br>
</br>
No se encontraron resultados.</br>

### Ejemplo 3: Múltiples resultados

Ingrese el nombre del jugador a buscar (o 'exit' para salir): Gen G Clid
</br>
[CLIENT] Enviando solicitud al servidor...</br>
[CLIENT] Esperando respuesta del servidor...</br>
</br>
================= PARTIDA =================</br>
Jugador buscado : Gen G Clid</br>
Duracion        : 22 min 03 s</br>
Resultado       : Ganó</br>
Participantes   :</br>
  1. ...</br>
  7. Gen G Clid</br>
  ...</br>
===========================================</br>
Enter = siguiente,  q = salir > [ENTER]</br>
</br>
================= PARTIDA =================</br>
Jugador buscado : Gen G Clid</br>
Duracion        : 31 min 45 s</br>
Resultado       : Perdió</br>
Participantes   :</br>
  ...</br>
===========================================</br>
Enter = siguiente,  q = salir > q</br>
</br>
Búsqueda interrumpida por el usuario. </br></br>

### Ejemplo 4: Salir del programa

Ingrese el nombre del jugador a buscar (o 'exit' para salir): exit
</br>
Cerrando cliente y servidor...</br>
¡Hasta luego! </br></br>


## Arquitectura del Sistema
![](/Diagram.png)

## Complejidad del Sistema

Construcción del índice: O(n log n) donde n = número total de jugadores</br>
Búsqueda por nombre: O(log n + k) donde k = colisiones del hash</br>
Espacio en disco: ~10 bytes por jugador en el índice</br>

## Índice ordenado por hash:

```cpp
sort(entries.begin(), entries.end(), LessEntry());
```

### Justificación: Permite búsqueda binaria O(log n) en vez de O(n)

### Ordenamiento secundario por offset para mantener orden de aparición</br>

## Búsqueda en dos fases:

### Fase 1: Búsqueda binaria del hash en el índice (lower_bound + upper_bound)
### Fase 2: Verificación del nombre real en el CSV (para resolver colisiones)
