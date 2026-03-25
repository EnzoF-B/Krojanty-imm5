#!/bin/bash

# Script de lancement pour l'interface GUI avec options réseau

case "$1" in
    -h|--help)
        echo "Usage:"
        echo "  $0                    # Interface graphique avec sélection de mode"
        echo "  $0 local              # Mode local (2 joueurs même clavier)"
        echo "  $0 server [port]      # Mode serveur (port par défaut: 8080)"
        echo "  $0 client host:port   # Mode client"
        echo "  $0 -h, --help         # Afficher cette aide"
        echo ""
        echo "Exemples:"
        echo "  $0 server 8080        # Serveur sur port 8080"
        echo "  $0 client localhost:8080  # Client vers localhost:8080"
        ;;
    local)
        echo "Lancement en mode local..."
        KROJANTY_MODE=local ./build/game_gui
        ;;
    server)
        PORT=${2:-8080}
        echo "Lancement en mode serveur sur le port $PORT..."
        KROJANTY_MODE=server KROJANTY_PORT=$PORT ./build/game_gui
        ;;
    client)
        if [ -z "$2" ]; then
            echo "Usage: $0 client <host:port>"
            echo "Exemple: $0 client localhost:8080"
            exit 1
        fi
        # Parser host:port
        IFS=':' read -r HOST PORT <<< "$2"
        if [ -z "$PORT" ]; then
            echo "Format invalide. Utilisez: host:port"
            echo "Exemple: localhost:8080"
            exit 1
        fi
        echo "Lancement en mode client vers $HOST:$PORT..."
        KROJANTY_MODE=client KROJANTY_HOST=$HOST KROJANTY_PORT=$PORT ./build/game_gui
        ;;
    "")
        echo "Lancement de l'interface graphique..."
        ./build/game_gui
        ;;
    *)
        echo "Option inconnue: $1"
        echo "Utilisez '$0 --help' pour voir les options disponibles"
        exit 1
        ;;
esac
