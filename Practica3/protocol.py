"""
Practica 3 - Streaming MP3 con UDP, hilos y tuberia
Materia: Aplicaciones para Comunicaciones en Web (6CM3)
ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)

Integrantes:
- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

Responsabilidad del archivo:
Define el contrato de protocolo (tipos de paquete, empaquetado/desempaquetado),
constantes de red y funciones de metadatos MP3 usadas por cliente y servidor.
"""

import json
import struct
from pathlib import Path

try:
    import miniaudio
except Exception:
    miniaudio = None


HOST = "127.0.0.1"
PORT = 5000
CHUNK_SIZE = 4096
WINDOW_SIZE = 8
TIMEOUT_SECONDS = 1.0
MAX_RETRIES = 20

PIPE_CHUNK_SIZE = 4096
PIPE_MAX_CHUNKS = 1024
PIPE_START_BYTES = 1024 * 1024

SONGS_DIR = Path("canciones")
DOWNLOADS_DIR = Path("descargadas")

REQUEST_LIST = b"LIST"
REQUEST_FILE_PREFIX = b"GET "
END_PACKET = -1

TYPE_META = b"M"
TYPE_DATA = b"D"
TYPE_END = b"E"
TYPE_ACK = b"A"
TYPE_ERROR = b"X"


def pack_json(kind, payload):
    return json.dumps({"kind": kind, "payload": payload}, ensure_ascii=False).encode("utf-8")


def unpack_json(data):
    message = json.loads(data.decode("utf-8"))
    return message["kind"], message["payload"]


def pack_meta(total_size):
    return TYPE_META + struct.pack("!Q", total_size)


def unpack_meta(packet):
    return struct.unpack("!Q", packet[1:9])[0]


def pack_data(number, chunk):
    return TYPE_DATA + struct.pack("!II", number, len(chunk)) + chunk


def unpack_data(packet):
    number, size = struct.unpack("!II", packet[1:9])
    return number, packet[9:9 + size]


def pack_ack(number):
    return TYPE_ACK + struct.pack("!i", number)


def unpack_ack(packet):
    return struct.unpack("!i", packet[1:5])[0]


def pack_end():
    return TYPE_END + struct.pack("!i", END_PACKET)


def pack_error(message):
    return TYPE_ERROR + message.encode("utf-8", errors="replace")


def unpack_error(packet):
    return packet[1:].decode("utf-8", errors="replace")


def safe_song_path(folder, filename):
    folder = Path(folder).resolve()
    candidate = (folder / filename).resolve()
    if folder not in candidate.parents or candidate.suffix.lower() != ".mp3":
        raise ValueError("Nombre de archivo no permitido")
    return candidate


def decode_text(raw):
    raw = raw.rstrip(b"\x00").strip()
    if not raw:
        return ""

    encoding = raw[0]
    payload = raw[1:]
    encodings = {
        0: "latin-1",
        1: "utf-16",
        2: "utf-16-be",
        3: "utf-8",
    }
    try:
        return payload.decode(encodings.get(encoding, "utf-8"), errors="replace").strip("\x00").strip()
    except Exception:
        return payload.decode("utf-8", errors="replace").strip("\x00").strip()


def syncsafe_to_int(data):
    value = 0
    for byte in data:
        value = (value << 7) | (byte & 0x7F)
    return value


def read_id3v2(path):
    metadata = {}
    frame_names = {
        "TIT2": "title",
        "TPE1": "artist",
        "TALB": "album",
        "TDRC": "year",
        "TYER": "year",
        "TCON": "genre",
    }

    with path.open("rb") as audio:
        header = audio.read(10)
        if len(header) != 10 or header[:3] != b"ID3":
            return metadata

        version = header[3]
        tag_size = syncsafe_to_int(header[6:10])
        data = audio.read(tag_size)

    pos = 0
    while pos + 10 <= len(data):
        frame_id = data[pos:pos + 4].decode("latin-1", errors="replace")
        if not frame_id.strip("\x00"):
            break

        if version == 4:
            frame_size = syncsafe_to_int(data[pos + 4:pos + 8])
        else:
            frame_size = int.from_bytes(data[pos + 4:pos + 8], "big")
        pos += 10

        frame_data = data[pos:pos + frame_size]
        pos += frame_size

        key = frame_names.get(frame_id)
        if key and frame_data:
            metadata[key] = decode_text(frame_data)

    return metadata


def read_id3v1(path):
    metadata = {}
    with path.open("rb") as audio:
        if path.stat().st_size < 128:
            return metadata
        audio.seek(-128, 2)
        tag = audio.read(128)

    if tag[:3] != b"TAG":
        return metadata

    fields = {
        "title": tag[3:33],
        "artist": tag[33:63],
        "album": tag[63:93],
        "year": tag[93:97],
    }
    for key, raw in fields.items():
        value = raw.rstrip(b"\x00 ").decode("latin-1", errors="replace").strip()
        if value:
            metadata[key] = value
    return metadata


def mp3_metadata(path):
    path = Path(path)
    metadata = read_id3v1(path)
    metadata.update({key: value for key, value in read_id3v2(path).items() if value})
    duration_seconds = 0
    if miniaudio:
        try:
            duration_seconds = int(miniaudio.mp3_get_file_info(str(path)).duration)
        except Exception:
            duration_seconds = 0
    return {
        "title": metadata.get("title") or path.stem,
        "artist": metadata.get("artist") or "Desconocido",
        "album": metadata.get("album") or "Desconocido",
        "year": metadata.get("year") or "N/D",
        "genre": metadata.get("genre") or "N/D",
        "duration_seconds": duration_seconds,
    }


def list_mp3_files(folder):
    folder = Path(folder)
    folder.mkdir(exist_ok=True)
    return sorted(
        (path for path in folder.iterdir() if path.is_file() and path.suffix.lower() == ".mp3"),
        key=lambda path: path.name.lower(),
    )
