# Aplicaciones para Comunicaciones en Red

Repositorio de prácticas de la materia **Aplicaciones para Comunicaciones en Red** — ESCOM.

## Prácticas

| # | Título | Tema | Estado |
|---|--------|------|--------|
| 1 | Servicio de transferencia de archivos | Sockets de flujo (TCP) | En progreso |

---

## Práctica 1 — Servicio de transferencia de archivos

Implementación de un sistema cliente-servidor de transferencia de archivos usando sockets TCP bloqueantes. El sistema permite subir, descargar, listar, borrar y renombrar archivos y carpetas entre un cliente y un servidor de forma remota.

### Integrantes

| Nombre | Responsabilidad |
|---|-----------------|
| Demian Romero Bautista | Cliente Java, protocolo de red |
| Said Ferreira | Servidor C |
| Mateo Alejandro Jaimes Uribe | Interfaz gráfica (Swing) |

### Arquitectura

El sistema usa dos canales TCP independientes:

**Canal de metadatos — puerto 8000 — permanente**
Se abre al iniciar la sesión y se mantiene abierto hasta que el usuario cierra el programa. Transporta mensajes JSON de una sola línea. Se usa para enviar comandos y recibir respuestas.

**Canal de datos — puerto 8001 — intermitente**
Se abre únicamente durante la transferencia de bytes y se cierra al terminar. Transporta bytes crudos sin modificar, lo que permite transferir cualquier tipo de archivo (imágenes, video, ejecutables, etc.) sin corrupción.

### Contrato del protocolo JSON

Todos los mensajes son objetos JSON en una sola línea, delimitados por `\n`. El cliente siempre envía primero y el servidor siempre responde.

**Comandos del cliente hacia el servidor:**

```text
{"cmd":"LIST"}
{"cmd":"UPLOAD","nombre":"foto.jpg","tamanio":204800}
{"cmd":"DOWNLOAD","nombre":"reporte.pdf"}
{"cmd":"DOWNLOAD_OK"}
{"cmd":"UPLOAD_DIR","nombre":"fotos","cantidad":3}
{"cmd":"FILE","nombre":"fotos/img.jpg","tamanio":81920}
{"cmd":"DOWNLOAD_DIR","nombre":"musica"}
{"cmd":"NEXT_OK"}
{"cmd":"DELETE","nombre":"viejo.txt"}
{"cmd":"RENAME_FILE","actual":"a.txt","nuevo":"b.txt"}
{"cmd":"RENAME_DIR","actual":"dir_a","nuevo":"dir_b"}
{"cmd":"EXIT"}
```

**Respuestas del servidor hacia el clientes:**

```text
{"status":"OK"}
{"status":"OK","tamanio":512000}
{"status":"OK","cantidad":4}
{"status":"UPLOAD_OK"}
{"status":"FILE_OK"}
{"status":"DELETE_OK"}
{"status":"RENAME_OK"}
{"status":"ITEM","tipo":"FILE","nombre":"x","tamanio":100}
{"status":"ITEM","tipo":"DIR","nombre":"fotos","tamanio":0}
{"status":"FIN_LIST"}
{"status":"NEXT","nombre":"cancion.mp3","tamanio":4200000}
{"status":"ERROR","msg":"Archivo no encontrado"}
```

> Cada línea es un mensaje JSON independiente (un objeto por línea, delimitado por `\n`).

**Regla de oro:** el cliente llama a `readLine()` exactamente una vez por cada comando enviado. La única excepción es `LIST`, que lee en bucle hasta recibir `FIN_LIST`.

### Implementación del cliente (Java)

El cliente se implementa exclusivamente en Java. Puntos clave:

- Conectar al puerto 8000 con `Socket` para el canal de metadatos.
- Cada mensaje JSON termina en `\n`. Usar `PrintWriter` para enviar y `BufferedReader.readLine()` para recibir.
- Para el canal de datos (puerto 8001) abrir un nuevo `Socket` antes de cada transferencia y cerrarlo al terminar.
- Leer exactamente `tamanio` bytes del canal de datos — no leer hasta EOF, ya que el socket permanece abierto para operaciones posteriores.
- Parsear y construir JSON con la librería Gson.

### Implementación del servidor (C)

El servidor se implementa exclusivamente en C. Debe respetar el mismo contrato JSON descrito arriba sin ninguna modificación. Puntos clave:

- Escuchar en el puerto 8000 con `socket()` + `bind()` + `listen()` + `accept()` para el canal de metadatos.
- Por cada cliente aceptado, leer comandos JSON línea por línea (`\n` como delimitador) usando `recv()` en un bucle hasta encontrar el salto de línea.
- Para el canal de datos (puerto 8001), aceptar una conexión entrante del mismo cliente justo antes de cada transferencia y cerrarla al terminar.
- Al recibir `UPLOAD`: aceptar la conexión en el puerto 8001 y leer exactamente `tamanio` bytes, escribiéndolos al archivo destino.
- Al recibir `DOWNLOAD`: abrir el archivo, enviar `{"status":"OK","tamanio":N}\n` por el canal de metadatos, luego abrir el canal de datos y enviar los bytes exactos del archivo.
- Para `LIST`: iterar el directorio de trabajo con `opendir()` / `readdir()`, enviar un mensaje `ITEM` por cada entrada y finalizar con `FIN_LIST`.
- Manejar `UPLOAD_DIR` / `DOWNLOAD_DIR` acumulando los archivos del directorio y transfiriéndolos uno por uno usando el flujo `FILE` / `NEXT` / `NEXT_OK`.
- Al recibir `EXIT`: cerrar el socket del cliente y volver a `accept()` para atender al siguiente.
- Para construir y parsear JSON en C se recomienda la librería [cJSON](https://github.com/DaveGamble/cJSON) (un solo archivo `.c` y `.h`, sin dependencias).

### Tecnologías

| Componente | Tecnología |
|------------|------------|
| Cliente | Java 21, `java.net`, `java.io` |
| Servidor | C estándar (C99), sockets POSIX |
| Protocolo | TCP, JSON sobre texto plano |
| Gestión de dependencias | Maven 3.x |
| Librería JSON (cliente) | Gson 2.10.1 (`com.google.code.gson`) |
| Librería JSON (servidor) | cJSON (archivo único, sin dependencias) |

### Estructura del proyecto

```
redes2-repo-practicas/
├── pom.xml
├── src/
│   └── main/
│       └── java/
│           └── practica1/
│               └── Cliente.java        # Cliente Java con protocolo JSON
└── servidor/
    ├── servidor.c                      # Servidor C con protocolo JSON
    ├── cJSON.c
    ├── cJSON.h
    └── Makefile
```

### Cómo ejecutar

**Iniciar el servidor C** (compilar y ejecutar):
```bash
cd servidor
make
./servidor
```

**Iniciar el cliente Java** (en otra terminal):
```bash
mvn compile
mvn exec:java -Dexec.mainClass="practica1.Cliente"
```

Por defecto el cliente se conecta a `127.0.0.1`. Para conectarse a otra máquina, cambiar la constante `SERVER_IP` en `Cliente.java` antes de compilar.

### Requisitos

- Java 21 o superior
- Maven 3.x
- GCC con soporte C99
- Conexión entre cliente y servidor en la misma red (o localhost para pruebas)