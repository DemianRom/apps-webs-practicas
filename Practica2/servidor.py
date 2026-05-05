import socket
from pathlib import Path

from protocol import (
    CHUNK_SIZE,
    HOST,
    MAX_RETRIES,
    PORT,
    REQUEST_FILE_PREFIX,
    REQUEST_LIST,
    SONGS_DIR,
    TYPE_ACK,
    TIMEOUT_SECONDS,
    WINDOW_SIZE,
    list_wav_files,
    pack_data,
    pack_end,
    pack_error,
    pack_json,
    pack_meta,
    safe_song_path,
    unpack_ack,
)


def send_with_ack(sock, packet, address, expected_ack):
    for attempt in range(1, MAX_RETRIES + 1):
        sock.sendto(packet, address)
        try:
            response, response_address = sock.recvfrom(1024)
        except socket.timeout:
            print(f"  Reintento {attempt}: no llego ACK[{expected_ack}]")
            continue

        if response_address == address and response[:1] == TYPE_ACK and unpack_ack(response) == expected_ack:
            return True

    return False


def send_song(sock, address, filename):
    try:
        song_path = safe_song_path(SONGS_DIR, filename)
    except ValueError as error:
        sock.sendto(pack_error(str(error)), address)
        return

    if not song_path.exists():
        sock.sendto(pack_error("La cancion no existe en el servidor"), address)
        return

    total_size = song_path.stat().st_size
    if not send_with_ack(sock, pack_meta(total_size), address, 0):
        sock.sendto(pack_error("Transferencia cancelada: no se confirmo la informacion inicial"), address)
        print("Transferencia cancelada: no llego ACK de metadata.", flush=True)
        return

    print(f"Enviando {song_path.name} ({total_size} bytes) a {address[0]}:{address[1]}")

    packets = []
    with song_path.open("rb") as song:
        packet_number = 0
        while chunk := song.read(CHUNK_SIZE):
            packets.append(pack_data(packet_number, chunk))
            packet_number += 1

    if not send_with_sliding_window(sock, packets, address):
        sock.sendto(pack_error("Transferencia cancelada: demasiados ACK perdidos"), address)
        print("Transferencia cancelada por falta de ACK.")
        return

    sock.sendto(pack_end(), address)
    print("Transferencia completa.\n")


def send_with_sliding_window(sock, packets, address):
    base = 0
    next_packet = 0
    retries = 0
    total_packets = len(packets)

    while base < total_packets:
        while next_packet < total_packets and next_packet < base + WINDOW_SIZE:
            sock.sendto(packets[next_packet], address)
            next_packet += 1

        try:
            response, response_address = sock.recvfrom(1024)
        except socket.timeout:
            retries += 1
            if retries > MAX_RETRIES:
                return False

            next_packet = base
            print(f"  Timeout. Reenviando ventana desde PKT[{base}]", flush=True)
            continue

        if response_address != address or response[:1] != TYPE_ACK:
            continue

        ack = unpack_ack(response)
        if ack >= base:
            base = ack + 1
            retries = 0

            if ack % 1000 == 0:
                print(f"  ACK acumulado hasta PKT[{ack}]", flush=True)

    return True


def send_song_list(sock, address):
    songs = [
        {"name": path.name, "size": path.stat().st_size}
        for path in list_wav_files(SONGS_DIR)
    ]
    sock.sendto(pack_json("LIST", songs), address)
    print(f"Lista enviada a {address[0]}:{address[1]} ({len(songs)} canciones)")


def run_server(host=HOST, port=PORT):
    Path(SONGS_DIR).mkdir(exist_ok=True)

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.bind((host, port))
        sock.settimeout(TIMEOUT_SECONDS)
        print(f"Servidor UDP escuchando en {host}:{port}")
        print(f"Carpeta de canciones: {Path(SONGS_DIR).resolve()}\n")

        while True:
            try:
                message, address = sock.recvfrom(8192)
            except socket.timeout:
                continue

            if message == REQUEST_LIST:
                send_song_list(sock, address)
            elif message.startswith(REQUEST_FILE_PREFIX):
                filename = message[len(REQUEST_FILE_PREFIX):].decode("utf-8", errors="replace").strip()
                send_song(sock, address, filename)
            else:
                sock.sendto(pack_error("Peticion desconocida"), address)


if __name__ == "__main__":
    run_server()
