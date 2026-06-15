# Práctica 5 — Servidor de Chat No Bloqueante en C++

Servidor de chat multiusuario con soporte de múltiples salas, historial persistente y cliente TUI
con ncurses. Implementado en C++17 para Linux usando `epoll` para I/O no bloqueante.

---

## Arquitectura

```
┌──────────────────────────────────────────────────────────────────────┐
│                          SERVIDOR (epoll)                            │
│                                                                      │
│  ┌───────────┐   ┌────────────┐   ┌──────────┐   ┌───────────────┐ │
│  │ ChatServer│──>│RoomManager │──>│  Sala    │──>│  logs/*.log   │ │
│  │  (epoll)  │   │ (map salas)│   │(miembros)│   │  (historial)  │ │
│  └───────────┘   └────────────┘   └──────────┘   └───────────────┘ │
│        │                                                             │
│  ┌─────┴──────┐                                                      │
│  │  Cliente   │  fd + nombre + sala_actual + buffer + cola_salida   │
│  └────────────┘                                                      │
└──────────────────────────────────────────────────────────────────────┘
          ↕ TCP JSON/\n
┌──────────────────────────────────────────────────────────────────────┐
│                          CLIENTE (ncurses)                           │
│                                                                      │
│  ┌──────────────┐  ┌─────────────────────────────────┐  ┌────────┐ │
│  │  Salas       │  │  Chat #general                  │  │ Input  │ │
│  │  #general ◀  │  │  [Alice] hola a todos           │  │ > _    │ │
│  │  #devs       │  │  * Bob se unió                  │  └────────┘ │
│  ├──────────────┤  │  [Bob]   qué tal                │             │
│  │  Usuarios    │  └─────────────────────────────────┘             │
│  │  * Alice     │                                                   │
│  │    Bob       │  Thread RX (recv) ─── Thread UI (ncurses+input)  │
│  └──────────────┘                                                   │
└──────────────────────────────────────────────────────────────────────┘
```

---

## Requisitos

- **OS:** Linux (Ubuntu 20.04+, Debian 11+)
- **Compilador:** g++ con soporte C++17 (`g++ --version`)
- **Dependencias:**
  ```bash
  sudo apt install build-essential libncurses5-dev curl
  ```

---

## Compilación

```bash
# Clonar / entrar al directorio
cd practica5/

# Compilar todo (descarga json.hpp automáticamente si no existe)
make all

# O compilar por separado
make server
make client

# Limpiar ejecutables
make clean
```

---

## Ejecución

### Iniciar el servidor

```bash
./server           # Puerto por defecto: 8080
./server 9000      # Puerto personalizado
```

### Conectar un cliente

```bash
./client 127.0.0.1 8080 Alice
./client 127.0.0.1 8080 Bob
```

Al iniciar, el cliente entra automáticamente a la sala `#general`.

---

## Comandos del cliente (TUI)

| Comando              | Descripción                              |
|----------------------|------------------------------------------|
| `<texto>`            | Enviar mensaje a la sala actual          |
| `@usuario`           | Mencionar a alguien (se le resalta)      |
| `/sala <nombre>`     | Unirse a una sala existente o nueva      |
| `/nueva <nombre>`    | Crear una sala nueva                     |
| `/salas`             | Listar todas las salas disponibles       |
| `/usuarios`          | Listar usuarios en la sala actual        |
| `/img <ruta>`        | Enviar una imagen a la sala (max 2MB)    |
| `/ver [n]`           | Ver una imagen recibida (n, o la última) |
| `/help`              | Mostrar este menú de ayuda               |
| `/salir`             | Salir del chat                           |
| `Tab`                | Cambiar a la siguiente sala              |
| `Ctrl+C`             | Salir de emergencia                      |

---

## Protocolo JSON

Los mensajes se intercambian como JSON terminado en `\n`. Ejemplos:

```json
// Cliente → Servidor
{ "tipo": "unirse", "sala": "general", "usuario": "Alice" }
{ "tipo": "mensaje", "sala": "general", "usuario": "Alice", "contenido": "hola 👋" }
{ "tipo": "crear_sala", "nombre": "devs", "usuario": "Alice" }
{ "tipo": "listar_salas" }

// Servidor → Cliente
{ "tipo": "broadcast", "sala": "general", "usuario": "Alice", "contenido": "hola 👋" }
{ "tipo": "sistema", "sala": "general", "contenido": "Bob se unió a la sala" }
{ "tipo": "lista_salas", "salas": ["general", "devs"] }
{ "tipo": "historial", "sala": "general", "mensajes": [...] }
```

Ver [docs/diagrama_protocolo.md](docs/diagrama_protocolo.md) para el diagrama completo.

---

## ¿Cómo funciona el diseño no bloqueante?

El servidor usa `epoll` en modo edge-triggered (`EPOLLET`):

1. El socket del servidor y todos los sockets de clientes se marcan como `O_NONBLOCK`.
2. Un único hilo ejecuta el loop `epoll_wait()` sin bloquearse.
3. Cuando llega un evento `EPOLLIN` en el socket del servidor → `accept()` en loop hasta `EAGAIN`.
4. Cuando llega `EPOLLIN` en un cliente → `recv()` en loop, acumula en buffer, parsea JSON por `\n`.
5. Si un `send()` devuelve `EAGAIN`, el mensaje se encola y se activa `EPOLLOUT` para ese fd.
6. `SIGINT` activa un `atomic<bool>` que termina el loop limpiamente cerrando todos los sockets.

El cliente usa **dos hilos**: uno para la TUI de ncurses (hilo principal) y otro para recibir
mensajes del servidor (`recv`), usando un `mutex` para sincronizar el estado compartido.

---

## Imágenes (Kitty Graphics Protocol)

Se pueden compartir imágenes en el chat con `/img <ruta>`. El cliente lee el archivo,
lo codifica en base64 y lo envía dentro de un mensaje JSON (`tipo: "imagen"`), que el
servidor reenvía a todos los miembros de la sala.

Al recibir una imagen, el cliente la decodifica a un archivo temporal en
`/tmp/chat_p5/` y muestra un marcador en el chat:

```
🖼  [0] foto.png (de Alice)  —  /ver 0
```

Con `/ver 0` (o `/ver` para la última) se visualiza la imagen:

- Si estás en la terminal **Kitty**, se renderiza inline con `kitty +kitten icat`
  (Kitty Graphics Protocol).
- En cualquier otra terminal, se abre con el visor del sistema (`xdg-open`).

Límite: 2 MB por imagen. Las imágenes no se guardan en el historial del servidor
(solo una nota ligera `Alice compartió la imagen: foto.png`).

## Menciones

Si escribes `@usuario` en un mensaje, ese nombre se resalta en amarillo para todos.
Cuando **a ti** te mencionan, tu línea completa se marca en negrita con una flecha `►`
al inicio, para que notes el ping aunque tengas muchos mensajes.

## Historial persistente

Cada sala tiene un archivo `logs/<sala>.log` en formato JSON Lines:

```
{"tipo":"broadcast","sala":"general","usuario":"Alice","contenido":"hola"}\n
```

Al iniciarse el servidor, carga el historial en memoria. Al unirse un nuevo usuario, recibe los
últimos 20 mensajes del historial de la sala.

---

## Estructura del proyecto

```
practica5/
├── Makefile
├── README.md
├── src/
│   ├── common/
│   │   ├── protocolo.hpp     ← tipos y helpers JSON
│   │   └── json.hpp          ← nlohmann/json (auto-descargado)
│   ├── server/
│   │   ├── main_server.cpp
│   │   ├── chat_server.hpp/.cpp  ← epoll loop + lógica
│   │   ├── sala.hpp/.cpp         ← sala de chat
│   │   └── cliente.hpp/.cpp      ← sesión de cliente conectado
│   └── client/
│       ├── main_client.cpp
│       ├── chat_client.hpp/.cpp  ← conexión TCP + thread RX
│       └── tui.hpp/.cpp          ← interfaz ncurses
├── logs/                         ← historial (creado en runtime)
└── docs/
    └── diagrama_protocolo.md
```

---

## Créditos

- **Materia:** Aplicaciones en Red
- **Institución:** ESCOM — IPN
- **Tecnologías:** C++17, epoll, POSIX sockets, nlohmann/json, ncurses
