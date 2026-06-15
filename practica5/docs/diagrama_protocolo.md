# Diagrama del Protocolo de Chat

## Flujo de conexión y mensajes

```
Cliente                          Servidor
  |                                 |
  |------- TCP connect ------------>|
  |                                 |  accept() + epoll add
  |<------ {tipo:"bienvenida"} -----|
  |<------ {tipo:"lista_salas"} ----|
  |                                 |
  |------- {tipo:"unirse",          |
  |          sala:"general",        |
  |          usuario:"Alice"} ----->|  Registra al cliente en la sala
  |                                 |  Carga historial del archivo
  |<------ {tipo:"historial"} ------|  (últimos 20 mensajes)
  |<------ {tipo:"lista_salas"} ----|
  |<------ {tipo:"lista_usuarios"}--|
  |                                 |
  |           (Bob también conectado y en #general)
  |                                 |
  |------- {tipo:"mensaje",         |
  |          contenido:"hola 👋"} ->|  Guarda en logs/general.log
  |<------ {tipo:"broadcast"} ------|  Echo al propio Alice
  |                    |            |
  |                   Bob           |
  |              <--- broadcast ----|  Reenvío a todos en #general
  |                                 |
  |------- {tipo:"crear_sala",      |
  |          nombre:"devs"} ------->|  Crea sala nueva en memoria
  |<- lista_salas (actualizada) ----|
  |                    Bob          |
  |              <- lista_salas ----|  Notificación a todos
  |                                 |
  |------- {tipo:"listar_salas"} -->|
  |<------ {tipo:"lista_salas"} ----|
  |                                 |
  |------- {tipo:"salir"} --------->|  Cierre limpio
  |                                 |  Notifica a la sala: "Alice salió"
  |<------- TCP close --------------|
```

## Tipos de mensajes

### Cliente → Servidor

| tipo            | Campos requeridos               | Descripción                        |
|-----------------|----------------------------------|------------------------------------|
| `unirse`        | `sala`, `usuario`               | Registrarse en una sala            |
| `crear_sala`    | `nombre`, `usuario`             | Crear nueva sala                   |
| `mensaje`       | `sala`, `usuario`, `contenido`  | Enviar mensaje a la sala           |
| `listar_salas`  | —                               | Solicitar lista de salas activas   |
| `listar_usuarios`| `sala` (opcional)              | Solicitar usuarios de una sala     |
| `salir`         | —                               | Desconectarse limpiamente          |

### Servidor → Cliente

| tipo             | Campos                                    | Descripción                          |
|------------------|-------------------------------------------|--------------------------------------|
| `bienvenida`     | `contenido`                              | Mensaje inicial al conectar          |
| `lista_salas`    | `salas: []`                              | Lista de salas disponibles           |
| `lista_usuarios` | `sala`, `usuarios: []`                   | Usuarios en una sala                 |
| `broadcast`      | `sala`, `usuario`, `contenido`           | Mensaje de un usuario                |
| `sistema`        | `sala`, `contenido`                      | Notificación del sistema             |
| `historial`      | `sala`, `mensajes: []`                   | Historial al entrar a una sala       |
| `error`          | `mensaje`                                | Descripción del error                |

## Diseño no bloqueante (epoll)

```
main_server.cpp
      │
      └──> ChatServer::run()
                │
                ├── epoll_wait() ──────────────────────────────────┐
                │      │                                           │
                │   evento en server_fd                       evento en client_fd
                │      │                                           │
                │   accept()                                  EPOLLIN: leer_de_cliente()
                │   set_nonblocking(fd)                            │
                │   epoll_ctl(ADD, EPOLLIN|EPOLLET)           acumular buffer
                │                                             parsear JSON por \n
                │                                             procesar_mensaje()
                │                                                  │
                │                                        EPOLLOUT: escribir_a_cliente()
                │                                             (si hay cola pendiente)
                │
                └── (vuelve a epoll_wait sin bloquearse)
```

## Framing de mensajes

Los mensajes se delimitan con `\n` (newline). Cada JSON es una línea:

```
{"tipo":"mensaje","sala":"general","usuario":"Alice","contenido":"hola"}\n
{"tipo":"broadcast","sala":"general","usuario":"Alice","contenido":"hola"}\n
```

Esto permite parsear el stream TCP aunque lleguen fragmentado o en bloques.
