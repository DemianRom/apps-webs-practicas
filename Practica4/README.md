# Practica 4 - Web crawler y mirror local

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Objetivo

Construir un crawler en C++ que recorra un sitio web, descargue sus recursos y genere un mirror local navegable. La practica trabaja con HTTP, parsing de enlaces, normalizacion de URLs, persistencia en disco y concurrencia con multiples hilos.

## Descripcion

El programa parte de una URL inicial, descubre enlaces internos, evita duplicados y descarga recursos del mismo host. Los archivos se guardan en una estructura local y los enlaces HTML/CSS se reescriben para poder navegar el sitio descargado desde un servidor local.

El crawler esta pensado para escenarios academicos como indices de directorio, sitios con enlaces relativos y recursos binarios.

## Estructura

```text
Practica4/
+-- Downloader.cpp
+-- Extractor.cpp
+-- Extractor.hpp
+-- README.md
```

Archivos principales:

- `Downloader.cpp`: logica principal, descarga con libcurl, pool de hilos, cola de trabajo y guardado local.
- `Extractor.cpp`: extraccion y normalizacion de URLs.
- `Extractor.hpp`: declaraciones del modulo extractor.

Los mirrors generados por el crawler no se conservan en el repositorio. Si necesitas evidencia de ejecucion, vuelve a generar una carpeta de salida con el comando de uso.

## Requisitos

En Linux o WSL:

```bash
sudo apt update
sudo apt install -y g++ libcurl4-openssl-dev
```

## Compilacion

Desde `Practica4/`:

```bash
g++ -std=c++17 Downloader.cpp Extractor.cpp -lcurl -pthread -o WebCrawler
```

## Uso

```bash
./WebCrawler <URL_INICIAL> [DIRECTORIO_SALIDA] [NUM_HILOS]
```

Ejemplo:

```bash
./WebCrawler http://148.204.58.221/axel/aplicaciones/ mirror_axel 8
```

## Funcionamiento general

1. Valida la URL inicial.
2. Normaliza la URL base.
3. Inicializa libcurl.
4. Crea una frontera compartida con cola y conjunto de visitados.
5. Lanza varios workers.
6. Cada worker toma una URL, descarga el recurso y lo guarda.
7. Si el recurso es parseable, extrae nuevos enlaces.
8. Filtra enlaces externos para permanecer en el mismo host.
9. Reescribe rutas para navegacion local.
10. Termina cuando la cola esta vacia y no hay workers activos.

## Concurrencia

La frontera del crawler usa:

- `queue<string>` para URLs pendientes.
- `set<string>` para URLs visitadas.
- `mutex` para proteger estado compartido.
- `condition_variable` para despertar workers.
- Contador de workers activos para detectar terminacion.

Esto evita descargar dos veces el mismo recurso y mantiene una salida ordenada aun con varios hilos.

## Mirror local

Para revisar una descarga:

```bash
cd mirror_axel
python3 -m http.server 8000 --bind 127.0.0.1
```

Luego abre:

```text
http://127.0.0.1:8000/
```

## Limitaciones

- No ejecuta JavaScript como navegador.
- No interpreta contenido generado dinamicamente en runtime.
- Se limita al mismo host por diseno.
- No implementa aun `robots.txt`, rate limiting fino ni reintentos avanzados.

## Aprendizajes

- Usar HTTP desde C++ con libcurl.
- Parsear y normalizar URLs reales.
- Resolver rutas relativas, query strings y recursos binarios.
- Coordinar un crawler multi-hilo.
- Pensar en limites de alcance para no descargar recursos externos sin control.
- Convertir una pagina remota en una copia local navegable.

## Valor dentro del semestre

Esta practica llevo los sockets y protocolos a un contexto mas cercano a la web real. Nos mostro que descargar una pagina no es solo pedir un HTML: tambien hay rutas, recursos, dominios, duplicados, errores y concurrencia.
