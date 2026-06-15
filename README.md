# Aplicaciones para Comunicaciones en Web

Repositorio de practicas de la unidad de aprendizaje **Aplicaciones para Comunicaciones en Web**, correspondiente al sexto semestre de la **Ingenieria en Sistemas Computacionales** en la **Escuela Superior de Computo (ESCOM)** del **Instituto Politecnico Nacional (IPN)**.

Este repositorio pertenece al grupo **6CM3**, impartido por el profesor **Axel Ernesto Moreno Cervantes**. Lo creamos para llevar de la mejor manera el desarrollo de nuestras practicas, mantener el codigo ordenado por entregas y dejar una base de consulta que tambien pueda servir como contribucion abierta en GitHub para estudiantes que esten repasando implementaciones de aplicaciones en red.

## Integrantes

- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

## Estructura

| Practica | Carpeta | Tema | Tecnologias | Estado |
|---|---|---|---|---|
| 1 | `Practica1/` | Servicio de transferencia de archivos con sockets de flujo | Java, C, TCP, JSON | Finalizada |
| 2 | `Practica2/` | Reproductor de musica con sockets de datagrama | Python, UDP, ventana deslizante | Finalizada |
| 3 | `Practica3/` | Streaming MP3 con hilos, tuberia y metadatos | Python, UDP, ventana deslizante, threads | Finalizada |
| 4 | `Practica4/` | Web Crawler, Mirroring| C++, Mutex, thread, https | Finalizada |

## Organizacion general

Cada practica vive en su propia carpeta y contiene su propio README con instrucciones de ejecucion, estructura y descripcion tecnica. La raiz del repositorio se mantiene como indice general para agregar futuras practicas sin mezclar codigo, reportes ni recursos.

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
|   +-- README.md
+-- Practica4/
|   +-- Downloader.cpp
|   +-- Extractor.cpp
|   +-- README.md
|   +-- Extractor.hpp
|   +-- README.md
+-- README.md
```

## Notas para nuevas practicas

- Crear una carpeta nueva con el formato `PracticaN/`.
- Agregar un `README.md` dentro de la carpeta de la practica.
- No subir archivos generados: compilados, caches, logs, carpetas `target/`, `__pycache__/`, ejecutables, objetos `.o` o descargas de prueba.
- Cuando una practica necesite archivos grandes de prueba, conservar solo la carpeta y documentar donde debe colocarlos el usuario.
