import socket
from pathlib import Path

from protocol import (
    DOWNLOADS_DIR,
    HOST,
    PORT,
    REQUEST_FILE_PREFIX,
    REQUEST_LIST,
    TYPE_DATA,
    TYPE_END,
    TYPE_ERROR,
    TYPE_META,
    TIMEOUT_SECONDS,
    WINDOW_SIZE,
    pack_ack,
    unpack_data,
    unpack_error,
    unpack_json,
    unpack_meta,
)


class MusicClient:
    def __init__(self, host=HOST, port=PORT, downloads_dir=DOWNLOADS_DIR):
        self.server = (host, port)
        self.downloads_dir = Path(downloads_dir)

    def list_songs(self):
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(TIMEOUT_SECONDS * 5)
            sock.sendto(REQUEST_LIST, self.server)
            response, _ = sock.recvfrom(65535)

        kind, songs = unpack_json(response)
        if kind != "LIST":
            raise RuntimeError("El servidor respondio algo inesperado")
        return songs

    def download_song(self, filename, progress=None):
        self.downloads_dir.mkdir(exist_ok=True)
        destination = self.downloads_dir / filename

        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
            sock.settimeout(TIMEOUT_SECONDS * 8)
            sock.sendto(REQUEST_FILE_PREFIX + filename.encode("utf-8"), self.server)

            first_packet, _ = sock.recvfrom(8192)
            if first_packet[:1] == TYPE_ERROR:
                raise RuntimeError(unpack_error(first_packet))
            if first_packet[:1] != TYPE_META:
                raise RuntimeError("No llego la informacion inicial de la cancion")

            total_size = unpack_meta(first_packet)
            sock.sendto(pack_ack(0), self.server)
            received = 0
            expected_packet = 0
            buffered_packets = {}

            with destination.open("wb") as output:
                while True:
                    packet, _ = sock.recvfrom(65535)

                    if packet[:1] == TYPE_ERROR:
                        raise RuntimeError(unpack_error(packet))

                    if packet[:1] == TYPE_END:
                        break

                    if packet[:1] != TYPE_DATA:
                        continue

                    packet_number, chunk = unpack_data(packet)
                    if expected_packet <= packet_number < expected_packet + WINDOW_SIZE:
                        buffered_packets[packet_number] = chunk

                    while expected_packet in buffered_packets:
                        chunk_to_write = buffered_packets.pop(expected_packet)
                        output.write(chunk_to_write)
                        received += len(chunk_to_write)
                        expected_packet += 1
                        if progress:
                            progress(received, total_size)

                    ack_number = expected_packet - 1
                    sock.sendto(pack_ack(ack_number), self.server)

        if destination.stat().st_size != total_size:
            destination.unlink(missing_ok=True)
            raise RuntimeError("La descarga quedo incompleta")

        return destination


def main():
    client = MusicClient()
    songs = client.list_songs()

    if not songs:
        print("El servidor no tiene canciones .wav.")
        return

    print("Canciones disponibles:")
    for index, song in enumerate(songs, start=1):
        mb = song["size"] / (1024 * 1024)
        print(f"  {index}. {song['name']} ({mb:.1f} MB)")

    while True:
        try:
            choice = int(input("Elige una cancion: "))
            if 1 <= choice <= len(songs):
                break
        except ValueError:
            pass
        print("Opcion invalida.")

    selected = songs[choice - 1]["name"]

    def show_progress(received, total):
        percent = int(received * 100 / total) if total else 0
        print(f"\rDescargando: {percent:3d}%", end="")

    path = client.download_song(selected, show_progress)
    print(f"\nDescarga completa: {path}")


if __name__ == "__main__":
    main()
