# Práctica 1 - Sistemas Operativos

Procesos y comunicación entre procesos.

</br>
## Integrantes

### Daniel Aguilar Castro - daaguilarc@unal.edu.co
### Andres Felipe Arias Gonzalez - anariasg@unal.edu.co
### Deiber David Gongora Hurtado - dgongora@unal.edu.co

</br>
## Descripción de la práctica

Usamos un dataset de 1Gb que guarda los datos de todas las partidas que se jugaron en 2020 en challenger korea para ejecutar una bùsqueda en menos de dos segundos usando como criterio el nombre de un jugador. Tècnicamente usamos dos procesos no emparentados que se comunican a travès de named pipes (una para enviar el criterio y otra para enviar la respuesta) La bùsqueda se hace usando una tabla hash indexada y ordenada, encontramos la posiciòn del hash usando bùsqueda binaria para posteriormente irnos directamente a la posiciòn de los datos en la csv.

</br>
## Campos utilizados:

### summonerName (string):

Nombre del invocador/jugador
Se encuentra dentro del JSON participantIdentities
Formato en CSV: 'summonerName': 'NombreJugador'
Hay 10 nombres por fila (10 jugadores por partida)
Propósito: Campo principal de búsqueda, se indexa con hash FNV-1a


### gameDuration (float):

Duración de la partida en segundos
Segunda columna del CSV (después del índice)
Propósito: Se muestra en minutos:segundos en los resultados


### participantId (int):

ID del jugador dentro de la partida (1-10)
Se encuentra en participantIdentities
Propósito: Relacionar el jugador con sus estadísticas


### win (boolean):

Indica si el jugador ganó o perdió
Se encuentra en participants[i].stats.win
Valores: True o False
Propósito: Determinar el resultado de la partida para ese jugador

</br>
## Implementación De Búsqueda Exacta

### Técnica de indexación: 
Hash Table con búsqueda binaria

### Decisiones de diseño:
Hash FNV-1a de 32 bits reducido a 16 bits:

    C++
if(line.find(name) == string::npos) continue;

### Uso de seekg() para acceso aleatorio:
No carga el dataset completo en memoria </br>
Solo lee las líneas necesarias usando offsets guardados

</br></br>

## Rangos de Valores Válidos

### Hash
hash16:           [0, 65535]        // uint16_t, módulo 65536

### Offset en CSV  
offset:           [0, 2^64-1]       // uint64_t, posición en bytes

### Datos extraídos
gameDuration:     [180.0, 7200.0]   // 3 min - 2 horas (estimado)
participantId:    [1, 10]           // Siempre 10 jugadores
win:              {True, False}     // Boolean

### summonerName
longitud:         [3, 16]           // Según límites de Riot Games
caracteres:       UTF-8             // Incluye coreano, chino, etc.

</br>
## Ejemplos Específicos de Uso del Programa

### Compilación:

    bash
make

Esto genera:
build_sorted_index → Construye el índice
server_search → Servidor de búsqueda
client_search → Cliente interactivo

### Ejecución completa:

    bash
make run

Esto:
Construye el índice (index_sorted.idx)
Inicia el servidor en background
Ejecuta el cliente
Cierra todo automáticamente

</br>
## Ejemplo 1: Búsqueda básica

bash: $ ./client_search
</br>
Ingrese el nombre del jugador a buscar (o 'exit' para salir): DWG Canyon
</br>
[CLIENT] Enviando solicitud al servidor...
[CLIENT] Esperando respuesta del servidor...
</br>
================= PARTIDA =================
Jugador buscado : DWG Canyon
Duracion        : 22 min 03 s
Resultado       : Ganó
Participantes   :
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
(offset CSV: 1234567)
===========================================
Enter = siguiente,  q = salir > 
</br></br>

### Ejemplo 2: Jugador no encontrado
</br>
Ingrese el nombre del jugador a buscar (o 'exit' para salir): FakerXD123
</br>
[CLIENT] Enviando solicitud al servidor...
[CLIENT] Esperando respuesta del servidor...
</br>
No se encontraron resultados.
</br></br>

### Ejemplo 3: Múltiples resultados
</br>
Ingrese el nombre del jugador a buscar (o 'exit' para salir): Gen G Clid
</br>
[CLIENT] Enviando solicitud al servidor...
[CLIENT] Esperando respuesta del servidor...
</br>
================= PARTIDA =================
Jugador buscado : Gen G Clid
Duracion        : 22 min 03 s
Resultado       : Ganó
Participantes   :
  1. ...
  7. Gen G Clid
  ...
===========================================
Enter = siguiente,  q = salir > [ENTER]
</br>
================= PARTIDA =================
Jugador buscado : Gen G Clid
Duracion        : 31 min 45 s
Resultado       : Perdió
Participantes   :
  ...
===========================================
Enter = siguiente,  q = salir > q
</br>
Búsqueda interrumpida por el usuario.
</br></br>

### Ejemplo 4: Salir del programa
</br>
Ingrese el nombre del jugador a buscar (o 'exit' para salir): exit
</br>
Cerrando cliente y servidor...
¡Hasta luego!
</br></br>


## Arquitectura del Sistema
(Driagram)

## Complejidad del Sistema

Construcción del índice: O(n log n) donde n = número total de jugadores
Búsqueda por nombre: O(log n + k) donde k = colisiones del hash
Espacio en disco: ~10 bytes por jugador en el índice

</br>
## Índice ordenado por hash:

    C++
sort(entries.begin(), entries.end(), LessEntry());

### Justificación: Permite búsqueda binaria O(log n) en vez de O(n)

### Ordenamiento secundario por offset para mantener orden de aparición

</br>
## Búsqueda en dos fases:

### Fase 1: Búsqueda binaria del hash en el índice (lower_bound + upper_bound)
### Fase 2: Verificación del nombre real en el CSV (para resolver colisiones)
