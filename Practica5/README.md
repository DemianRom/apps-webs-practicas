# Practica 5 - Servidor de chat no bloqueante

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Objetivo

Implementar un chat multiusuario en C++ con sockets TCP no bloqueantes. El servidor usa `epoll` para atender multiples clientes sin crear un hilo por conexion, y el cliente usa una interfaz TUI con `ncurses`.

## Descripcion

La practica permite que varios usuarios se conecten a un servidor, entren a salas, envien mensajes, consulten historial y compartan imagenes pequenas. Los mensajes se intercambian como JSON terminado en salto de linea.

Caracteristicas principales:

- Servidor TCP no bloqueante con `epoll`.
- Multiples clientes simultaneos.
- Salas de chat.
- Historial persistente por sala.
- Cliente TUI con `ncurses`.
- Menciones a usuarios.
- Envio de imagenes codificadas en base64.

## Estructura

```text
Practica5/
+-- Makefile
+-- README.md
+-- docs/
|   +-- diagrama_protocolo.md
+-- src/
    +-- common/
    |   +-- base64.hpp
    |   +-- protocolo.hpp
    +-- server/
    |   +-- main_server.cpp
    |   +-- chat_server.hpp
    |   +-- chat_server.cpp
    |   +-- sala.hpp
    |   +-- sala.cpp
    |   +-- cliente.hpp
    |   +-- cliente.cpp
    +-- client/
        +-- main_client.cpp
        +-- chat_client.hpp
        +-- chat_client.cpp
        +-- tui.hpp
        +-- tui.cpp
```

## Requisitos

Linux o WSL:

```bash
sudo apt install build-essential libncurses5-dev curl
```

## Compilacion

```bash
cd Practica5
make all
```

Tambien se puede compilar por separado:

```bash
make server
make client
```

Limpiar binarios:

```bash
make clean
```

## Ejecucion

Servidor:

```bash
./server
./server 9000
```

Clientes:

```bash
./client 127.0.0.1 8080 Demian
./client 127.0.0.1 8080 Said
```

Al conectarse, el cliente entra a la sala `general`.

## Comandos del cliente

| Comando | Descripcion |
|---|---|
| `<texto>` | Enviar mensaje a la sala actual |
| `@usuario` | Mencionar a alguien |
| `/sala <nombre>` | Unirse a una sala |
| `/nueva <nombre>` | Crear una sala |
| `/salas` | Listar salas disponibles |
| `/usuarios` | Listar usuarios de la sala actual |
| `/img <ruta>` | Enviar imagen de hasta 2 MB |
| `/ver [n]` | Ver una imagen recibida |
| `/help` | Mostrar ayuda |
| `/salir` | Salir del chat |
| `Tab` | Cambiar a la siguiente sala |
| `Ctrl+C` | Salida de emergencia |

## Protocolo

Los mensajes viajan como JSON terminado en `\n`.

Ejemplos:

```json
{ "tipo": "unirse", "sala": "general", "usuario": "Demian" }
{ "tipo": "mensaje", "sala": "general", "usuario": "Said", "contenido": "hola" }
{ "tipo": "crear_sala", "nombre": "devs", "usuario": "Demian" }
{ "tipo": "listar_salas" }
```

Consulta `docs/diagrama_protocolo.md` para el diagrama del protocolo.

## Diseno no bloqueante

El servidor usa `epoll` en modo edge-triggered:

1. El socket servidor y los sockets de clientes se configuran como `O_NONBLOCK`.
2. El loop principal espera eventos con `epoll_wait()`.
3. En eventos de entrada del socket servidor, acepta clientes hasta recibir `EAGAIN`.
4. En eventos de entrada de cliente, lee todo lo disponible y arma mensajes por `\n`.
5. Si un `send()` no puede escribir todo, el mensaje queda en cola y se espera `EPOLLOUT`.
6. Al cerrar, se limpian sockets y estructuras.

El cliente usa dos hilos:

- Hilo principal para `ncurses` y entrada del usuario.
- Hilo receptor para mensajes del servidor.

## Historial e imagenes

Cada sala persiste mensajes en `logs/<sala>.log` usando JSON Lines. Cuando un usuario entra, recibe los ultimos mensajes de la sala.

Las imagenes se codifican en base64 y se reenvian como mensajes JSON. El cliente las decodifica en `/tmp/chat_p5/` y permite abrirlas con `/ver`.

## Aprendizajes

- Disenar un servidor que atiende multiples clientes sin bloquearse.
- Usar `epoll` y sockets no bloqueantes.
- Manejar buffers parciales de entrada y salida.
- Persistir historial por sala.
- Integrar protocolo JSON, TUI, archivos e imagenes.
- Comparar un modelo orientado a eventos contra un modelo de un hilo por cliente.

## Valor dentro del semestre

Esta practica cierra el curso con una aplicacion completa: usuarios reales, mensajes en vivo, estado compartido, persistencia y entrada/salida no bloqueante. Fue una sintesis de todo lo aprendido sobre aplicaciones de red.
