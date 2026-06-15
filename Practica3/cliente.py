"""
Practica 3 - Streaming MP3 con UDP, hilos y tuberia
Materia: Aplicaciones y Comunicaciones en Red (6CM1)
Periodo: 26/2
ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)

Integrantes:
- Romero Bautista Demian
- Ferreira Rodriguez Said

Responsabilidad del archivo:
Contiene la logica del cliente: descarga UDP con control de flujo,
pipeline de audio con AudioPipe y control de reproduccion interna.
"""

import socket
import threading
import time
from dataclasses import dataclass, field
from pathlib import Path

import miniaudio

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


def emit(callback, *args):
    if callback:
        callback(*args)


class AudioPipe:
    """Tuberia con cache compartido entre el hilo de descarga y el hilo reproductor."""

    def __init__(self, max_chunks=PIPE_MAX_CHUNKS):
        self.max_bytes = max_chunks * 4096
        self.closed = threading.Event()
        self.bytes_written = 0
        self.bytes_read = 0
        self.lock = threading.Lock()
        self.data_ready = threading.Condition(self.lock)
        self.cache = bytearray()

    def write(self, chunk):
        if self.closed.is_set():
            return False
        with self.data_ready:
            self.cache.extend(chunk)
            self.bytes_written += len(chunk)
            self.data_ready.notify_all()
        return True

    def read_at(self, position, num_bytes):
        with self.data_ready:
            while position >= len(self.cache) and not self.closed.is_set():
                self.data_ready.wait(timeout=0.1)
            if position >= len(self.cache):
                return b""
            end = min(position + num_bytes, len(self.cache))
            return bytes(self.cache[position:end])

    def set_playback_position(self, position):
        with self.lock:
            self.bytes_read = max(0, min(position, self.bytes_written))

    def close(self):
        self.closed.set()
        with self.data_ready:
            self.data_ready.notify_all()

    def buffered_chunks(self):
        return self.buffered_bytes() // 4096

    def buffered_bytes(self):
        with self.lock:
            return max(0, self.bytes_written - self.bytes_read)

    def downloaded_bytes(self):
        with self.lock:
            return self.bytes_written

    def stats(self):
        with self.lock:
            buffered = max(0, self.bytes_written - self.bytes_read)
            return {
                "downloaded": self.bytes_written,
                "played": self.bytes_read,
                "buffered": buffered,
                "chunks": buffered // 4096,
            }


class AudioPipeSource(miniaudio.StreamableSource):
    """Fuente secuencial para que miniaudio decodifique MP3 directamente desde la tuberia."""

    def __init__(self, audio_pipe):
        self.audio_pipe = audio_pipe
        self.position = 0

    def read(self, num_bytes):
        data = self.audio_pipe.read_at(self.position, num_bytes)
        self.position += len(data)
        self.audio_pipe.set_playback_position(self.position)
        return data

    def seek(self, offset, origin):
        if origin == miniaudio.SeekOrigin.START:
            self.position = max(0, offset)
        elif origin == miniaudio.SeekOrigin.CURRENT:
            self.position = max(0, self.position + offset)
        elif origin == miniaudio.SeekOrigin.END:
            self.position = max(0, self.audio_pipe.downloaded_bytes() + offset)
        self.audio_pipe.set_playback_position(self.position)
        return True

    def close(self):
        self.audio_pipe.close()


@dataclass
class StreamResult:
    path: Path | None = None
    error: Exception | None = None
    done: threading.Event = field(default_factory=threading.Event)


@dataclass
class StreamSession:
    result: StreamResult
    transfer_thread: threading.Thread
    playback_thread: threading.Thread
    pipe: AudioPipe

    def cancel(self):
        self.pipe.close()


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
            sock.settimeout(TIMEOUT_SECONDS)
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
                        if audio_pipe.closed.is_set():
                            raise RuntimeError("Transferencia cancelada por el usuario")

                        try:
                            packet, _ = sock.recvfrom(65535)
                        except socket.timeout:
                            if audio_pipe.closed.is_set():
                                raise RuntimeError("Transferencia cancelada por el usuario")
                            continue

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
                            if not audio_pipe.write(chunk_to_write):
                                raise RuntimeError("La tuberia fue cerrada antes de terminar la transferencia")
                            received += len(chunk_to_write)
                            expected_packet += 1
                            emit(progress, received, total_size)
                            stats = audio_pipe.stats()
                            emit(pipe_status, stats["chunks"], stats["buffered"])

                        sock.sendto(pack_ack(expected_packet - 1), self.server)
            finally:
                audio_pipe.close()

        if destination.stat().st_size != total_size:
            destination.unlink(missing_ok=True)
            raise RuntimeError("La descarga quedo incompleta")

        return destination

    def start_streaming(self, filename, player, progress=None, pipe_status=None, playback_status=None, position_status=None):
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
                player.play_from_pipe(audio_pipe, filename, playback_status, position_status)
            except Exception as error:
                if result.error is None:
                    result.error = error
            finally:
                result.done.set()

        transfer_thread = threading.Thread(target=transfer_worker, name="hilo-transferencia", daemon=True)
        playback_thread = threading.Thread(target=playback_worker, name="hilo-reproduccion", daemon=True)
        transfer_thread.start()
        playback_thread.start()
        return StreamSession(result, transfer_thread, playback_thread, audio_pipe)


class AudioPlayer:
    def __init__(self, playback_dir=DOWNLOADS_DIR):
        self.playback_dir = Path(playback_dir)
        self.stop_requested = threading.Event()
        self.engine = MiniaudioPipeEngine()
        self.current_pipe = None

    def stop(self):
        self.stop_requested.set()
        if self.current_pipe:
            self.current_pipe.close()
        self.engine.stop()

    def pause(self):
        return self.engine.pause()

    def resume(self):
        return self.engine.resume()

    def seek_to(self, target_seconds):
        return self.engine.seek_to(target_seconds)

    def is_playing(self):
        return self.engine.is_playing()

    def stop_audio(self):
        self.engine.stop()

    def play_downloaded_file(self, path, status=None, position_status=None):
        self.stop_requested.clear()
        self.engine.stop()
        ok, message = self.engine.play_file_simple(Path(path), self.stop_requested, status, position_status)
        if status:
            if ok:
                status("Reproduccion de archivo descargado finalizada", 0)
            else:
                status(f"No se pudo reproducir descargada: {message}", 0)
        return ok

    def play_from_pipe(self, audio_pipe, filename, status=None, position_status=None):
        self.stop_requested.clear()
        self.engine.stop()
        self.current_pipe = audio_pipe
        self.playback_dir.mkdir(exist_ok=True)

        while audio_pipe.buffered_bytes() < PIPE_START_BYTES and not self.stop_requested.is_set():
            emit(status, "Buffer inicial de reproduccion", audio_pipe.buffered_bytes())
            if audio_pipe.closed.is_set():
                break
            time.sleep(0.05)

        bytes_available = audio_pipe.buffered_bytes()
        if not self.stop_requested.is_set() and bytes_available > 0:
            emit(status, "Iniciando reproductor desde tuberia", bytes_available)
            ok, _message = self.engine.play_pipe(audio_pipe, self.stop_requested, status, position_status)
        else:
            ok = False

        if self.stop_requested.is_set():
            emit(status, "Reproduccion detenida", bytes_available)
        elif ok:
            emit(status, "Reproduccion finalizada", audio_pipe.bytes_read)
        else:
            emit(status, "Tuberia cerrada sin datos suficientes", bytes_available)
        self.current_pipe = None


class MiniaudioPipeEngine:
    """Reproductor MP3 interno que decodifica directamente desde AudioPipe."""

    def __init__(self):
        self.device = None
        self.playing = False
        self.paused = False
        self.finished = threading.Event()
        self.lock = threading.Lock()
        self.sample_rate = 44100
        self.channels = 2
        self.bytes_per_sample = 2
        self.played_frames = 0
        self.seek_request_frames = None
        self.last_position_report = 0

    def play_pipe(self, audio_pipe, stop_event, status=None, position_status=None):
        source = AudioPipeSource(audio_pipe)
        self.finished.clear()

        try:
            playback_stream = self._start_device(
                lambda seek_frame: self._pipe_decoder(source, seek_frame),
                stop_event,
                position_status,
                status,
                "Reproduciendo desde tuberia mientras descarga continua",
                audio_pipe.bytes_read,
            )
            return playback_stream
        except Exception as error:
            with self.lock:
                self.playing = False
                self.paused = False
                self.device = None
            return False, str(error)

    def play_file(self, path, stop_event, status=None, position_status=None):
        path = Path(path).resolve()
        self.finished.clear()

        try:
            return self._start_device(
                lambda seek_frame: self._file_decoder(path, seek_frame),
                stop_event,
                position_status,
                status,
                "Reproduciendo archivo descargado",
                0,
            )
        except Exception as error:
            with self.lock:
                self.playing = False
                self.paused = False
                self.device = None
            return False, str(error)

    def play_file_simple(self, path, stop_event, status=None, position_status=None):
        path = Path(path).resolve()
        self.finished.clear()

        try:
            decoded_stream = miniaudio.stream_file(
                str(path),
                output_format=miniaudio.SampleFormat.SIGNED16,
                nchannels=self.channels,
                sample_rate=self.sample_rate,
                frames_to_read=1024,
            )
            playback_stream = self._file_playback_stream(decoded_stream, stop_event, position_status)
            next(playback_stream)
            device = miniaudio.PlaybackDevice(
                output_format=miniaudio.SampleFormat.SIGNED16,
                nchannels=self.channels,
                sample_rate=self.sample_rate,
                buffersize_msec=250,
                app_name="Practica 3",
            )
            with self.lock:
                self.device = device
                self.playing = True
                self.paused = False
                self.played_frames = 0
                self.last_position_report = 0
            device.start(playback_stream)
            emit(status, "Reproduciendo archivo descargado", 0)

            while not stop_event.is_set() and not self.finished.is_set():
                time.sleep(0.1)

            with self.lock:
                device_to_close = self.device
                self.device = None
                self.playing = False
                self.paused = False
            if device_to_close:
                device_to_close.close()
            return True, "Reproduccion terminada"
        except Exception as error:
            with self.lock:
                self.playing = False
                self.paused = False
                self.device = None
            return False, str(error)

    def _start_device(self, decoder_factory, stop_event, position_status, status, start_message, status_bytes):
        try:
            decoded_stream = decoder_factory(0)
            playback_stream = self._controlled_stream(decoder_factory, decoded_stream, stop_event, position_status)
            next(playback_stream)
            device = miniaudio.PlaybackDevice(
                output_format=miniaudio.SampleFormat.SIGNED16,
                nchannels=self.channels,
                sample_rate=self.sample_rate,
                buffersize_msec=250,
                app_name="Practica 3",
            )
            with self.lock:
                self.device = device
                self.playing = True
                self.paused = False
                self.played_frames = 0
                self.seek_request_frames = None
                self.last_position_report = 0
            device.start(playback_stream)
            if status:
                status(start_message, status_bytes)

            while not stop_event.is_set() and not self.finished.is_set():
                time.sleep(0.1)

            device.close()
            with self.lock:
                self.device = None
                self.playing = False
                self.paused = False
            return True, "Reproduccion terminada"
        except Exception as error:
            with self.lock:
                self.playing = False
                self.paused = False
                self.device = None
            return False, str(error)

    def _pipe_decoder(self, source, seek_frame):
        return miniaudio.stream_any(
            source,
            source_format=miniaudio.FileFormat.MP3,
            output_format=miniaudio.SampleFormat.SIGNED16,
            nchannels=self.channels,
            sample_rate=self.sample_rate,
            frames_to_read=1024,
            seek_frame=seek_frame,
        )

    def _file_decoder(self, path, seek_frame):
        return miniaudio.stream_file(
            str(path),
            output_format=miniaudio.SampleFormat.SIGNED16,
            nchannels=self.channels,
            sample_rate=self.sample_rate,
            frames_to_read=1024,
            seek_frame=seek_frame,
        )

    def _controlled_stream(self, decoder_factory, decoded_stream, stop_event, position_status=None):
        frame_count = yield b""
        try:
            while not stop_event.is_set():
                if self.paused:
                    frame_count = yield b"\x00" * (frame_count * self.channels * self.bytes_per_sample)
                    continue

                with self.lock:
                    seek_frame = self.seek_request_frames
                    self.seek_request_frames = None

                if seek_frame is not None:
                    decoded_stream = decoder_factory(seek_frame)
                    self.played_frames = seek_frame
                    self._report_position(position_status, seeking=True)

                samples = decoded_stream.send(frame_count)
                self.played_frames += frame_count
                self._report_position(position_status, seeking=False)
                frame_count = yield samples
        except StopIteration:
            self.finished.set()

    def _file_playback_stream(self, decoded_stream, stop_event, position_status=None):
        frame_count = yield b""
        try:
            while not stop_event.is_set():
                if self.paused:
                    frame_count = yield b"\x00" * (frame_count * self.channels * self.bytes_per_sample)
                    continue

                samples = decoded_stream.send(frame_count)
                self.played_frames += frame_count
                self._report_position(position_status, seeking=False)
                frame_count = yield samples
        except StopIteration:
            self.finished.set()

    def _report_position(self, position_status, seeking=False):
        if not position_status:
            return

        seconds = int(self.played_frames / self.sample_rate)
        if seconds != self.last_position_report or seeking:
            self.last_position_report = seconds
            position_status(seconds, seeking)

    def stop(self):
        with self.lock:
            device = self.device
            self.device = None
            self.playing = False
            self.paused = False
            self.finished.set()
        if device:
            try:
                device.stop()
            except Exception:
                pass
            try:
                device.close()
            except Exception:
                pass

    def pause(self):
        with self.lock:
            if not self.playing:
                return False, "No hay audio activo"
            if self.paused:
                return True, "El audio ya estaba pausado"
            self.paused = True
            return True, "Audio pausado"

    def resume(self):
        with self.lock:
            if not self.playing:
                return False, "No hay audio activo"
            if not self.paused:
                return True, "El audio ya estaba reproduciendose"
            self.paused = False
            return True, "Audio reanudado"

    def is_playing(self):
        with self.lock:
            return self.playing

    def seek_to(self, target_seconds):
        target_seconds = max(0, int(target_seconds))
        target_frames = target_seconds * self.sample_rate
        with self.lock:
            if not self.playing:
                return False, "No hay audio activo"
            self.seek_request_frames = target_frames
            self.paused = False
            return True, f"Moviendo reproduccion a {target_seconds} s"


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
    player = AudioPlayer()

    def progress(received, total):
        percent = int(received * 100 / total) if total else 0
        print(f"\rTransferencia UDP: {percent:3d}%", end="")

    def pipe_status(chunks, bytes_buffered):
        if bytes_buffered and bytes_buffered % (512 * 1024) < 4096:
            print(f"\nTuberia: {chunks} chunks, {bytes_buffered} bytes pendientes")

    def playback_status(status, bytes_available):
        print(f"\nHilo de reproduccion: {status} ({bytes_available} bytes)")

    session = client.start_streaming(
        selected,
        player,
        progress=progress,
        pipe_status=pipe_status,
        playback_status=playback_status,
    )
    while session.transfer_thread.is_alive() or session.playback_thread.is_alive():
        time.sleep(0.2)

    if session.result.error:
        print(f"\nError: {session.result.error}")
    else:
        print(f"\nDescarga completa: {session.result.path}")


if __name__ == "__main__":
    main()
