#!/usr/bin/env python3
import socket
import os

SOCK_PATH = "/tmp/monitor.sock"

# 1. Si ya existía, lo borras para poder bindear
if os.path.exists(SOCK_PATH):
    os.unlink(SOCK_PATH)

# 2. Creas el socket UNIX y lo asocias a la ruta
server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(SOCK_PATH)

# 3. Empiezas a escuchar (solo 1 conexión en cola)
server.listen(1)
print(f"[+] Servidor escuchando en {SOCK_PATH}")

# 4. Esperas a que QEMU se conecte
conn, _ = server.accept()
print("[+] QEMU se ha conectado al monitor")

# 5. A partir de aquí, conn.recv() trae lo que QEMU mande,
#    y conn.sendall(...) te permite enviar comandos.
#    Si no quieres procesar nada, basta con mantenerlo abierto.
try:
    while True:
        data = conn.recv(1024)
        if not data:
            break
        # (opcional) imprimimos la salida
        print(data.decode(), end="")
finally:
    conn.close()
    server.close()
    os.unlink(SOCK_PATH)
    print("[+] Servidor cerrado")
