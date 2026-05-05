import json
import struct
from pathlib import Path


HOST = "127.0.0.1"
PORT = 5000
CHUNK_SIZE = 4096
WINDOW_SIZE = 8
TIMEOUT_SECONDS = 1.0
MAX_RETRIES = 20

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
    data = json.dumps({"kind": kind, "payload": payload}, ensure_ascii=False).encode("utf-8")
    return data


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
    chunk = packet[9:9 + size]
    return number, chunk


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
    if folder not in candidate.parents or candidate.suffix.lower() != ".wav":
        raise ValueError("Nombre de archivo no permitido")
    return candidate


def list_wav_files(folder):
    folder = Path(folder)
    folder.mkdir(exist_ok=True)
    return sorted(
        (path for path in folder.iterdir() if path.is_file() and path.suffix.lower() == ".wav"),
        key=lambda path: path.name.lower(),
    )
