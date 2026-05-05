import ctypes
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
        self.started_internal_player = False
        self.engine = MciMp3Engine()
        self.current_pipe = None

    def stop(self):
        self.stop_requested.set()
        if self.current_pipe:
            self.current_pipe.close()
        self.engine.stop()
        self.engine.close()

    def pause(self):
        return self.engine.pause()

    def resume(self):
        return self.engine.resume()

    def stop_audio(self):
        self.stop_requested.set()
        if self.current_pipe:
            self.current_pipe.close()
        self.engine.stop()
        self.engine.close()

    def play_from_pipe(self, audio_pipe, filename, status=None):
        self.stop_requested.clear()
        self.engine.close()
        self.started_internal_player = False
        self.current_pipe = audio_pipe
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

                if not self.started_internal_player and bytes_available >= PIPE_START_BYTES:
                    self.started_internal_player = self.start_internal_player(status)

        if not self.started_internal_player and bytes_available > 0:
            self.started_internal_player = self.start_internal_player(status)

        if status:
            if self.stop_requested.is_set():
                status("Audio interno detenido; tuberia cerrada", bytes_available)
            elif self.started_internal_player:
                status("Tuberia consumida; reproductor interno activo", bytes_available)
            else:
                status("Tuberia consumida; audio listo en archivo", bytes_available)
        self.current_pipe = None

    def start_internal_player(self, status=None):
        if not self.playback_file or not self.playback_file.exists():
            return False

        ok, message = self.engine.play_file(self.playback_file)
        if status:
            if ok:
                status("Reproductor interno MCI reproduciendo MP3", self.playback_file.stat().st_size)
            else:
                status(f"Esperando buffer reproducible: {message}", self.playback_file.stat().st_size)
        return ok


class MciMp3Engine:
    """Reproductor MP3 interno en Windows usando MCI, sin abrir apps externas."""

    def __init__(self):
        self.winmm = ctypes.WinDLL("winmm")
        self.alias = "practica3_mp3"
        self.opened = False
        self.paused_position = 0

    def _send(self, command):
        buffer = ctypes.create_unicode_buffer(512)
        code = self.winmm.mciSendStringW(command, buffer, len(buffer), None)
        if code:
            error_buffer = ctypes.create_unicode_buffer(512)
            self.winmm.mciGetErrorStringW(code, error_buffer, len(error_buffer))
            return False, error_buffer.value or f"Codigo MCI {code}"
        return True, buffer.value

    def play_file(self, path):
        self.close()
        path = Path(path).resolve()
        ok, message = self._send(f'open "{path}" type mpegvideo alias {self.alias}')
        if not ok:
            return False, message

        self.opened = True
        self.paused_position = 0
        self._send(f"set {self.alias} time format milliseconds")
        ok, message = self._send(f"play {self.alias}")
        if not ok:
            self.close()
            return False, message
        return True, "Reproduciendo"

    def stop(self):
        if self.opened:
            self._send(f"stop {self.alias}")

    def pause(self):
        if not self.opened:
            return False, "No hay audio activo"
        ok, position = self.position()
        if ok:
            self.paused_position = position
        ok, message = self._send(f"stop {self.alias}")
        if not ok:
            return False, message
        return True, f"Pausado en {self.paused_position} ms"

    def resume(self):
        if not self.opened:
            return False, "No hay audio activo"
        return self._send(f"play {self.alias} from {self.paused_position}")

    def position(self):
        ok, value = self._send(f"status {self.alias} position")
        if not ok:
            return False, 0
        try:
            return True, int(value.strip() or "0")
        except ValueError:
            return True, 0

    def close(self):
        if self.opened:
            self._send(f"close {self.alias}")
            self.opened = False
            self.paused_position = 0


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
