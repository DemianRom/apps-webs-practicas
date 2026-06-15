# Diagrama del protocolo de chat

Este documento resume el flujo de mensajes entre el cliente TUI y el servidor no bloqueante de la Practica 5.

## Flujo de conexion

```text
Cliente                                Servidor
   |                                      |
   | ---------- TCP connect ------------> |
   |                                      | accept() + epoll_ctl(ADD)
   | <--------- bienvenida ------------- |
   | <--------- lista_salas ------------ |
   |                                      |
   | ---------- unirse ----------------> | registra usuario en sala
   |                                      | carga historial de logs/<sala>.log
   | <--------- historial -------------- |
   | <--------- lista_usuarios --------- |
   |                                      |
   | ---------- mensaje ----------------> | guarda y reenvia
   | <--------- broadcast -------------- |
   |                         Bob <------ | broadcast a miembros de la sala
   |                                      |
   | ---------- crear_sala ------------> | crea sala en memoria
   | <--------- lista_salas ------------ |
   |                         Bob <------ | notificacion de salas
   |                                      |
   | ---------- salir -----------------> | cierre limpio
   | <--------- TCP close -------------- |
```

## Mensajes del cliente al servidor

| Tipo | Campos principales | Descripcion |
|---|---|---|
| `unirse` | `sala`, `usuario` | Entrar a una sala. |
| `crear_sala` | `nombre`, `usuario` | Crear una sala nueva. |
| `mensaje` | `sala`, `usuario`, `contenido` | Enviar mensaje a la sala actual. |
| `imagen` | `sala`, `usuario`, `nombre_archivo`, `datos` | Enviar imagen codificada en base64. |
| `listar_salas` | ninguno | Pedir salas disponibles. |
| `listar_usuarios` | `sala` | Pedir usuarios de una sala. |
| `salir` | ninguno | Desconectarse limpiamente. |

## Mensajes del servidor al cliente

| Tipo | Campos principales | Descripcion |
|---|---|---|
| `bienvenida` | `contenido` | Mensaje inicial al conectar. |
| `lista_salas` | `salas` | Lista de salas disponibles. |
| `lista_usuarios` | `sala`, `usuarios` | Usuarios dentro de una sala. |
| `broadcast` | `sala`, `usuario`, `contenido` | Mensaje reenviado a una sala. |
| `imagen` | `sala`, `usuario`, `nombre_archivo`, `datos` | Imagen reenviada a la sala. |
| `sistema` | `sala`, `contenido` | Evento del sistema. |
| `historial` | `sala`, `mensajes` | Ultimos mensajes al entrar a una sala. |
| `error` | `mensaje` | Error de protocolo o validacion. |

## Framing sobre TCP

TCP entrega un flujo de bytes, no mensajes completos. Por eso cada mensaje JSON se termina con `\n`.

```text
{"tipo":"mensaje","sala":"general","usuario":"Demian","contenido":"hola"}\n
{"tipo":"broadcast","sala":"general","usuario":"Demian","contenido":"hola"}\n
```

El servidor acumula bytes en el buffer de cada cliente hasta encontrar saltos de linea. Cada linea completa se parsea como JSON independiente.

## Ciclo no bloqueante con epoll

```text
main_server.cpp
    |
    v
ChatServer::run()
    |
    v
epoll_wait()
    |
    +-- Evento en server_fd
    |      +-- accept() hasta EAGAIN
    |      +-- set_nonblocking(fd)
    |      +-- epoll_ctl(ADD, EPOLLIN | EPOLLET)
    |
    +-- Evento EPOLLIN en client_fd
    |      +-- recv() hasta EAGAIN
    |      +-- acumular bytes
    |      +-- separar por \n
    |      +-- procesar mensajes JSON
    |
    +-- Evento EPOLLOUT en client_fd
           +-- enviar cola pendiente
           +-- desactivar EPOLLOUT si la cola queda vacia
```

## Persistencia

Cada sala guarda historial en:

```text
logs/<sala>.log
```

El formato es JSON Lines: un mensaje JSON por linea. Esto permite cargar los ultimos mensajes al iniciar el servidor o cuando un usuario entra a una sala.
