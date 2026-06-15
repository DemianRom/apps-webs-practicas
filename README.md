# Aplicaciones y Comunicaciones en Red

Repositorio final de practicas de la unidad de aprendizaje **Aplicaciones y Comunicaciones en Red**, impartida por el profesor **Axel Ernesto Moreno Cervantes** en el grupo **6CM1**, periodo **26/2**.

Este trabajo reune las cinco practicas desarrolladas durante el semestre por:

- Demian Romero Bautista
- Said Ferreira Rodriguez

El objetivo de este repositorio es dejar una memoria tecnica clara del curso: que se pueda consultar, ejecutar, estudiar y recordar como fuimos construyendo aplicaciones de red cada vez mas completas.

## Vision del semestre

Durante el curso pasamos de usar sockets para transferir archivos a construir aplicaciones con control de flujo, concurrencia, mirroring web y comunicacion no bloqueante. Cada practica agrega una pieza importante:

- Entender el modelo cliente-servidor y separar metadatos de datos.
- Implementar confiabilidad basica sobre UDP con paquetes, ACK y ventana deslizante.
- Coordinar hilos para transferir, reproducir y actualizar interfaces sin bloquear la aplicacion.
- Descargar y reconstruir sitios web con parsing, normalizacion de URLs y trabajo concurrente.
- Disenar un servidor de chat con multiples clientes, salas, historial y sockets no bloqueantes.

Mas que una coleccion de programas, este repositorio muestra el camino completo: protocolo, transporte, sincronizacion, persistencia, interfaz y documentacion.

## Practicas

| Practica | Carpeta | Tema central | Tecnologias principales | Estado |
|---|---|---|---|---|
| 1 | `Practica1/` | Servicio de transferencia de archivos | C, Java, TCP, JSON, Swing | Finalizada |
| 2 | `Practica2/` | Reproductor y descarga por UDP | Python, UDP, sliding window, Tkinter | Finalizada |
| 3 | `Practica3/` | Streaming con hilos y tuberia | Python, UDP, threads, Condition, miniaudio | Finalizada |
| 4 | `Practica4/` | Web crawler y mirror local | C++17, libcurl, threads, mutex | Finalizada |
| 5 | `Practica5/` | Chat no bloqueante multiusuario | C++17, epoll, TCP, JSON, ncurses | Finalizada |

## Estructura general

```text
redes2-repo-practicas/
+-- Practica1/
|   +-- Cliente/
|   +-- Servidor/
|   +-- docs/
|   +-- README.md
+-- Practica2/
|   +-- cliente.py
|   +-- servidor.py
|   +-- gui.py
|   +-- protocol.py
|   +-- README.md
+-- Practica3/
|   +-- cliente.py
|   +-- servidor.py
|   +-- gui.py
|   +-- protocol.py
|   +-- requirements.txt
|   +-- README.md
+-- Practica4/
|   +-- Downloader.cpp
|   +-- Extractor.cpp
|   +-- Extractor.hpp
|   +-- README.md
+-- Practica5/
|   +-- Makefile
|   +-- docs/
|   +-- src/
|   +-- README.md
+-- README.md
```

## Aprendizajes generales

- **Protocolos propios:** aprendimos a definir comandos, respuestas, errores, tamanos, ACK y mensajes estructurados.
- **Sockets TCP y UDP:** vimos cuando conviene una conexion confiable y cuando tiene sentido controlar manualmente la transferencia.
- **Control de flujo:** implementamos ventana deslizante, paquetes numerados, retransmision y confirmaciones acumuladas.
- **Concurrencia:** usamos hilos, locks, condiciones, colas thread-safe y modelos no bloqueantes para evitar esperas innecesarias.
- **Persistencia y recursos:** guardamos archivos, mirrors, logs e historiales para que las aplicaciones mantuvieran estado util.
- **Interfaz y experiencia:** integramos GUI/TUI para que los sistemas fueran demostrables y no solo programas de consola.
- **Documentacion tecnica:** dejamos instrucciones, arquitectura y decisiones para que otra persona pueda continuar el trabajo.

## Como usar este repositorio

Cada practica tiene su propio `README.md` con:

- Objetivo.
- Estructura.
- Requisitos.
- Instrucciones de ejecucion.
- Resumen tecnico.
- Aprendizajes principales.

Para probar una practica, entra a su carpeta y sigue su README. Algunas practicas estan pensadas para Linux o WSL porque usan sockets POSIX, `epoll`, `ncurses` o `libcurl`.

## Prueba rapida por practica

Esta guia resume la forma mas directa de validar cada entrega. Para detalles completos, consulta el README de cada carpeta.

| Practica | Comandos base | Resultado esperado |
|---|---|---|
| `Practica1/` | `cd Practica1/Servidor && make && ./servidor` y en otra terminal `cd Practica1/Cliente && mvn exec:java -Dexec.mainClass="practica1.ClienteGUI"` | Cliente Java conectado al servidor C para gestionar archivos. |
| `Practica2/` | `cd Practica2 && python servidor.py` y en otra terminal `python gui.py` | GUI con lista de canciones y descarga por UDP. |
| `Practica3/` | `cd Practica3 && pip install -r requirements.txt && python servidor.py --delay-ms 15` y en otra terminal `python gui.py` | Streaming MP3 con progreso, cache y reproduccion durante descarga. |
| `Practica4/` | `cd Practica4 && g++ -std=c++17 Downloader.cpp Extractor.cpp -lcurl -pthread -o WebCrawler` | Binario `WebCrawler` compilado para generar mirrors locales. |
| `Practica5/` | `cd Practica5 && make all`, luego `./server` y clientes con `./client 127.0.0.1 8080 Nombre` | Chat multiusuario con salas desde la TUI. |

## Nota de cierre

Este repositorio queda como evidencia del semestre y como material de consulta para estudiantes que quieran entender implementaciones reales de aplicaciones en red. Hay codigo perfectible, decisiones de practica y partes hechas para demostracion, pero justamente eso lo vuelve valioso: muestra el proceso completo de aprender construyendo.
