#!/usr/bin/env python3
import sys
import ujson as json
import csv
import io
import os

def format_duration(seconds):
    minutes = int(seconds // 60)
    secs = int(seconds % 60)
    return f"{minutes}:{secs:02d}"

def convert_python_to_json(s):
    return s.replace("'", '"').replace('True', 'true').replace('False', 'false').replace('None', 'null')

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
    
    summoner_name = sys.argv[1]
    
    input_fd = int(os.environ.get('INPUT_FD'))
    output_fd = int(os.environ.get('OUTPUT_FD'))
    
    results = []
    
    input_data = b""
    
    while True:
        try:
            chunk = os.read(input_fd, 1048576)
            if not chunk:
                break
            input_data += chunk
        except OSError:
            break
    
    lines = input_data.decode('utf-8').split('\n')
    
    for line in lines:
        line = line.strip()
        if not line:
            continue
        
        try:
            partes = next(csv.reader([line]))
            
            if len(partes) < 10:
                continue
            
            duracion_segundos = float(partes[2])
            duracion_formateada = format_duration(duracion_segundos)
            
            identities = json.loads(convert_python_to_json(partes[8]))
            participants = json.loads(convert_python_to_json(partes[9]))
            
            id_a_nombre = {identity['participantId']: identity['player']['summonerName'] for identity in identities}
            
            player_found = False
            equipo_azul = []
            equipo_rojo = []
            player_stats = None
            resultado = None
            
            for participant in participants:
                pid = participant['participantId']
                team_id = participant['teamId']
                nombre = id_a_nombre.get(pid)
                
                if team_id == 100:
                    equipo_azul.append(nombre)
                else:
                    equipo_rojo.append(nombre)
                
                if nombre == summoner_name:
                    player_found = True
                    player_stats = participant['stats']
                    resultado = 1 if player_stats['win'] else 0
            
            if not player_found:
                continue
            
            kills = player_stats['kills']
            deaths = player_stats['deaths']
            assists = player_stats['assists']
            kda = (kills + assists) / deaths if deaths > 0 else (kills + assists)
            
            output = [
                "",
                "=" * 60,
                "Resultado: ✓ VICTORIA" if resultado == 1 else "Resultado: ✗ DERROTA",
                f"Duración: {duracion_formateada}",
                "=" * 60,
                "",
                "EQUIPO AZUL (100):",
                *[f"  ► {j}" if j == summoner_name else f"    {j}" for j in equipo_azul],
                "",
                "EQUIPO ROJO (200):",
                *[f"  ► {j}" if j == summoner_name else f"    {j}" for j in equipo_rojo],
                "",
                f"ESTADÍSTICAS DE {summoner_name}:",
                "-" * 60,
                f"K/D/A:             {kills}/{deaths}/{assists} (KDA: {kda:.2f})",
                f"CS:                {player_stats['totalMinionsKilled'] + player_stats['neutralMinionsKilled']}",
                f"Gold:              {player_stats['goldEarned']}",
                f"Daño a campeones:  {player_stats['totalDamageDealtToChampions']}",
                f"Daño recibido:     {player_stats['totalDamageTaken']}",
                f"Vision Score:      {player_stats['visionScore']}",
                f"Nivel final:       {player_stats['champLevel']}",
                "=" * 60,
            ]
            
            results.append("\n".join(output))
            
        except:
            continue
    
    if results:
        output_text = "\nNEXT_MATCH\n".join(results)
        os.write(output_fd, output_text.encode('utf-8'))
    
    sys.exit(0)