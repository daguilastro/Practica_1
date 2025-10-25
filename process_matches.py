#!/usr/bin/env python3
import sys
import json
import unicodedata
import csv
import io

def format_duration(seconds):
    """Convierte segundos a formato MM:SS"""
    minutes = int(seconds // 60)
    secs = int(seconds % 60)
    return f"{minutes}:{secs:02d}"

if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit(1)
    
    summoner_name = sys.argv[1]
    results = []
    
    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        
        try:
            reader = csv.reader(io.StringIO(line))
            partes = next(reader)
            
            if len(partes) < 9:
                continue
            
            duracion_segundos = float(partes[2])
            duracion_formateada = format_duration(duracion_segundos)
            identities_str = partes[8]
            participants_str = partes[9]
            
            # Convertir formato Python a JSON
            identities_str = identities_str.replace("'", '"').replace('True', 'true').replace('False', 'false').replace('None', 'null')
            participants_str = participants_str.replace("'", '"').replace('True', 'true').replace('False', 'false').replace('None', 'null')
            
            identities = json.loads(identities_str)
            participants = json.loads(participants_str)
            
            # Crear mapeo de ID a nombre
            id_a_nombre = {}
            for identity in identities:
                pid = identity['participantId']
                nombre = identity['player']['summonerName']
                id_a_nombre[pid] = nombre
            
            # Buscar el jugador y separar equipos
            player_participant_id = None
            equipo_azul = []
            equipo_rojo = []
            player_stats = None
            resultado = None
            
            for participant in participants:
                pid = participant['participantId']
                team_id = participant['teamId']
                nombre = id_a_nombre[pid]
                
                if team_id == 100:
                    equipo_azul.append(nombre)
                else:
                    equipo_rojo.append(nombre)
                
                if nombre == summoner_name:
                    player_participant_id = pid
                    player_stats = participant['stats']
                    resultado = 1 if player_stats['win'] else 0
            
            if player_participant_id is None:
                continue
            
            # Extraer estadísticas
            kills = player_stats['kills']
            deaths = player_stats['deaths']
            assists = player_stats['assists']
            gold_earned = player_stats['goldEarned']
            total_damage_to_champions = player_stats['totalDamageDealtToChampions']
            total_damage_taken = player_stats['totalDamageTaken']
            vision_score = player_stats['visionScore']
            cs = player_stats['totalMinionsKilled'] + player_stats['neutralMinionsKilled']
            level = player_stats['champLevel']
            kda = (kills + assists) / deaths if deaths > 0 else (kills + assists)
            
            # Formatear salida sin bordes
            output = []
            output.append("")
            output.append("=" * 60)
            
            if resultado == 1:
                output.append("Resultado: ✓ VICTORIA")
            else:
                output.append("Resultado: ✗ DERROTA")
            
            output.append(f"Duración: {duracion_formateada}")
            output.append("=" * 60)
            output.append("")
            
            output.append("EQUIPO AZUL (100):")
            for jugador in equipo_azul:
                if jugador == summoner_name:
                    output.append(f"  ► {jugador}")
                else:
                    output.append(f"    {jugador}")
            
            output.append("")
            output.append("EQUIPO ROJO (200):")
            for jugador in equipo_rojo:
                if jugador == summoner_name:
                    output.append(f"  ► {jugador}")
                else:
                    output.append(f"    {jugador}")
            
            output.append("")
            output.append(f"ESTADÍSTICAS DE {summoner_name}:")
            output.append("-" * 60)
            output.append(f"K/D/A:             {kills}/{deaths}/{assists} (KDA: {kda:.2f})")
            output.append(f"CS:                {cs}")
            output.append(f"Gold:              {gold_earned}")
            output.append(f"Daño a campeones:  {total_damage_to_champions}")
            output.append(f"Daño recibido:     {total_damage_taken}")
            output.append(f"Vision Score:      {vision_score}")
            output.append(f"Nivel final:       {level}")
            output.append("=" * 60)
            
            results.append("\n".join(output))
            
        except:
            continue
    
    if results:
        print("\nNEXT_MATCH\n".join(results))  # ← MANTENER EL SEPARADOR
    
    sys.exit(0)