import os
import queue
import socket
import threading
import time
from pathlib import Path

from protocol import (
    DOWNLOADS_DIR,
    HOST,
    PIPE_MAX_CHUNKS,
    PIPE_START_BYTES,
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


class AudioPipe:
    """Tuberia con buffer grande para comunicar transferencia y reproduccion."""

    def __init__(self, max_chunks=PIPE_MAX_CHUNKS):
        self.queue = queue.Queue(maxsize=max_chunks)
        self.closed = False
        self.bytes_written = 0
        self.bytes_read = 0

    def write(self, chunk):
        if self.closed:
            return
        self.queue.put(chunk)
        self.bytes_written += len(chunk)

    def read(self):
        chunk = self.queue.get()
        if chunk is None:
            return b""
        self.bytes_read += len(chunk)
        return chunk

    def close(self):
        if not self.closed:
            self.closed = True
            self.queue.put(None)

    def buffered_chunks(self):
        return self.queue.qsize()

    def buffered_bytes(self):
        return max(0, self.bytes_written - self.bytes_read)


class StreamResult:
    def __init__(self):
        self.path = None
        self.error = None
        self.done = threading.Event()


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

    def stream_song(self, filename, audio_pipe, progress=None, pipe_status=None):
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

            try:
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
                            audio_pipe.write(chunk_to_write)
                            received += len(chunk_to_write)
                            expected_packet += 1
                            if progress:
                                progress(received, total_size)
                            if pipe_status:
                                pipe_status(audio_pipe.buffered_chunks(), audio_pipe.buffered_bytes())

                        sock.sendto(pack_ack(expected_packet - 1), self.server)
            finally:
                audio_pipe.close()

        if destination.stat().st_size != total_size:
            destination.unlink(missing_ok=True)
            raise RuntimeError("La descarga quedo incompleta")

        return destination

    def start_streaming(self, filename, player, progress=None, pipe_status=None, playback_status=None):
        audio_pipe = AudioPipe()
        result = StreamResult()

        def transfer_worker():
            try:
                result.path = self.stream_song(filename, audio_pipe, progress, pipe_status)
            except Exception as error:
                result.error = error
                audio_pipe.close()
            finally:
                result.done.set()

        def playback_worker():
            try:
                player.play_from_pipe(audio_pipe, filename, playback_status)
            except Exception as error:
                if result.error is None:
                    result.error = error
            finally:
                result.done.set()

        transfer_thread = threading.Thread(target=transfer_worker, name="hilo-transferencia", daemon=True)
        playback_thread = threading.Thread(target=playback_worker, name="hilo-reproduccion", daemon=True)
        transfer_thread.start()
        playback_thread.start()
        return result, transfer_thread, playback_thread, audio_pipe


class PipeMp3Player:
    def __init__(self, playback_dir=DOWNLOADS_DIR):
        self.playback_dir = Path(playback_dir)
        self.stop_requested = threading.Event()
        self.playback_file = None
        self.started_external_player = False

    def stop(self):
        self.stop_requested.set()

    def play_from_pipe(self, audio_pipe, filename, status=None):
        self.stop_requested.clear()
        self.playback_dir.mkdir(exist_ok=True)
        self.playback_file = self.playback_dir / f"stream_{filename}"
        bytes_available = 0

        with self.playback_file.open("wb") as output:
            while not self.stop_requested.is_set():
                chunk = audio_pipe.read()
                if not chunk:
                    break

                output.write(chunk)
                output.flush()
                bytes_available += len(chunk)
                if status:
                    status("Hilo de reproduccion leyendo tuberia", bytes_available)

                if not self.started_external_player and bytes_available >= PIPE_START_BYTES:
                    self.open_external_player()
                    self.started_external_player = True

        if not self.started_external_player and bytes_available > 0:
            self.open_external_player()
            self.started_external_player = True

        if status:
            status("Reproduccion alimentada por tuberia finalizada", bytes_available)

    def open_external_player(self):
        if self.playback_file and self.playback_file.exists():
            try:
                os.startfile(str(self.playback_file))
            except OSError:
                pass


def main():
    client = MusicClient()
    songs = client.list_songs()

    if not songs:
        print("El servidor no tiene canciones .mp3.")
        return

    print("Canciones disponibles:")
    for index, song in enumerate(songs, start=1):
        mb = song["size"] / (1024 * 1024)
        print(f"  {index}. {song['name']} ({mb:.1f} MB) - {song.get('artist')} / {song.get('album')}")

    while True:
        try:
            choice = int(input("Elige una cancion: "))
            if 1 <= choice <= len(songs):
                break
        except ValueError:
            pass
        print("Opcion invalida.")

    selected = songs[choice - 1]["name"]
    player = PipeMp3Player()

    def progress(received, total):
        percent = int(received * 100 / total) if total else 0
        print(f"\rTransferencia UDP: {percent:3d}%", end="")

    result, transfer_thread, playback_thread, _ = client.start_streaming(selected, player, progress)
    while transfer_thread.is_alive() or playback_thread.is_alive():
        time.sleep(0.2)

    if result.error:
        print(f"\nError: {result.error}")
    else:
        print(f"\nDescarga completa: {result.path}")


if __name__ == "__main__":
    main()
