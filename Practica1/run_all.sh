#!/usr/bin/env bash
# Practica 1 - Servicio de transferencia de archivos
# Materia: Aplicaciones y Comunicaciones en Red (6CM1)
# ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
# Periodo: 26/2
#
# Integrantes:
# - Romero Bautista Demian
# - Ferreira Rodriguez Said
#
# Responsabilidad del archivo:
# Script auxiliar para iniciar o coordinar la ejecucion de los componentes principales de la practica.

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
SERV_DIR="$ROOT_DIR/Servidor"
CLIENT_DIR="$ROOT_DIR/Cliente"
LOGFILE="$SERV_DIR/server.log"

# Trap to ensure server is killed when script exits
SERVER_PID=0
cleanup() {
  if [ "$SERVER_PID" -ne 0 ] && kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Stopping server (pid $SERVER_PID)"
    kill "$SERVER_PID" || true
    sleep 0.2
  fi
}
trap cleanup EXIT

echo "Building server..."
cd "$SERV_DIR"
make || true
chmod +x servidor || true

# Start server in background if not already listening on port 8000
if ss -ltn | grep -q ":8000"; then
  echo "Server already listening on port 8000; not starting a new instance."
else
  echo "Starting servidor (stdout -> $LOGFILE)"
  nohup ./servidor > "$LOGFILE" 2>&1 &
  SERVER_PID=$!
  echo "Servidor PID: $SERVER_PID"
fi

# Wait for server to accept connections (timeout 10s)
echo "Waiting for server to listen on ports 8000/8001..."
for i in {1..20}; do
  if ss -ltn | grep -q ":8000" && ss -ltn | grep -q ":8001"; then
    echo "Server is up."
    break
  fi
  sleep 0.5
done

# Start the Java client GUI in foreground
echo "Starting Cliente GUI (Maven exec). Press Ctrl-C to exit GUI."
cd "$CLIENT_DIR"
mvn -DskipTests compile exec:java -Dexec.mainClass=practica1.ClienteGUI

# When the GUI exits, cleanup trap will run
echo "GUI exited."
exit 0
