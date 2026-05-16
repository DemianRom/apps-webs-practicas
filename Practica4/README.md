# Práctica 4 - WebCrawler / Mirror Local (C++ + libcurl, Multi-hilo)

Este proyecto implementa un **crawler/mirror local** en C++ que:

- Parte de una URL inicial.
- Descubre enlaces de forma recursiva.
- Evita duplicados con `visited`.
- Descarga recursos del **mismo host**.
- Reescribe rutas para servir contenido desde local.
- Ejecuta el crawling en **múltiples hilos** de forma **thread-safe**.

---

## Tabla de contenido

1. [Objetivo](#objetivo)
2. [Estructura del proyecto](#estructura-del-proyecto)
3. [Requisitos](#requisitos)
4. [Compilación](#compilación)
5. [Uso](#uso)
6. [Cómo funciona internamente](#cómo-funciona-internamente)
7. [Concurrencia y thread-safety](#concurrencia-y-thread-safety)
8. [Descubrimiento y normalización de URLs](#descubrimiento-y-normalización-de-urls)
9. [Guardado local y reescritura de enlaces](#guardado-local-y-reescritura-de-enlaces)
10. [Ejemplos de ejecución](#ejemplos-de-ejecución)
11. [Verificación del mirror local](#verificación-del-mirror-local)
12. [Problemas comunes y soluciones](#problemas-comunes-y-soluciones)
13. [Limitaciones actuales](#limitaciones-actuales)
14. [Mejoras sugeridas](#mejoras-sugeridas)

---

## Objetivo

Construir un descargador recursivo de sitios web con enfoque de mirror:

- **Recorrido automático** del sitio.
- **Cobertura amplia** de recursos (HTML, CSS, JS, imágenes, PDFs, PPT/PPTX, etc.).
- **Persistencia local** en estructura de carpetas.
- **Ejecución eficiente** con workers concurrentes.
- **Seguridad operativa** evitando salir a otros hosts.

---

## Estructura del proyecto

Archivos principales:

- `Downloader.cpp`
  - Lógica principal del crawler.
  - Descarga con libcurl.
  - Pool de hilos.
  - Cola/visitados thread-safe.
  - Reescritura de rutas para mirror local.

- `Extractor.cpp`
  - Utilidades de parsing/normalización de URL.
  - Extracción base de enlaces (`href/src`).
  - Resolución de rutas relativas (`..`, `.`).

- `Extractor.hpp`
  - Declaraciones públicas del módulo extractor.

Salida típica:

- `mirror/` (o carpeta que indiques en el comando)
  - Contiene el sitio descargado localmente.

---

## Requisitos

En Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y g++ libcurl4-openssl-dev
```

Herramientas usadas:

- `g++` (C++17)
- `libcurl`
- Biblioteca estándar de C++ (`thread`, `mutex`, `condition_variable`, `filesystem`, `regex`, etc.)

---

## Compilación

Desde la carpeta `Practica4`:

```bash
g++ -std=c++17 Downloader.cpp Extractor.cpp -lcurl -pthread -o WebCrawler
```

Parámetros importantes:

- `-lcurl`: enlaza libcurl.
- `-pthread`: soporte de hilos y sincronización.

---

## Uso

```bash
./WebCrawler <URL_INICIAL> [DIRECTORIO_SALIDA] [NUM_HILOS]
```

Donde:

- `URL_INICIAL`: URL de inicio (`http://...` o `https://...`).
- `DIRECTORIO_SALIDA` (opcional): carpeta mirror (default: `mirror`).
- `NUM_HILOS` (opcional): número de workers (default: `hardware_concurrency`, fallback a 4).

Ejemplo:

```bash
./WebCrawler https://site.com mirror_site 8
```

---

## Cómo funciona internamente

Flujo de alto nivel:

1. Validación de argumentos y protocolo (`http/https`).
2. Normalización de URL inicial.
3. Inicialización de libcurl global.
4. Creación de frontera compartida (cola + visitados + control de trabajo activo).
5. Lanzamiento de `N` workers.
6. Cada worker:
   - Toma una URL de la cola.
   - Descarga contenido.
   - Determina tipo por `Content-Type`.
   - Guarda recurso localmente.
   - Extrae nuevos enlaces.
   - Normaliza + filtra por host.
   - Encola solo URLs nuevas.
7. Terminación limpia cuando:
   - Cola vacía **y**
   - No hay workers activos.

---

## Concurrencia y thread-safety

### Estructura clave: `FronteraCrawler`

Contiene:

- `queue<string> cola_`
- `set<string> visitados_`
- `size_t activos_`
- `bool detenido_`
- `mutex mutex_`
- `condition_variable cv_`

### Garantías

- No hay `front/pop/push` sin lock.
- No hay inserciones simultáneas inseguras en `visited`.
- El cierre del sistema se decide bajo lock (`cola vacía + activos == 0`).
- Los logs se serializan con mutex para evitar salida mezclada.

Esto previene condiciones de carrera en la frontera compartida.

---

## Descubrimiento y normalización de URLs

### Fuentes de enlaces detectadas

Para aumentar cobertura, el crawler extrae URLs desde:

- HTML:
  - `href`, `src`, `poster`, `data-src`, `data-href`
  - `srcset`
  - `url(...)` embebido
  - URLs en texto (`http(s)://...` y `/ruta/...`)
- CSS:
  - `url(...)`
  - `@import`
  - URLs en texto
- JS/textos parseables:
  - URLs absolutas y relativas root (`/...`) dentro de strings

### Soporte de atributos con y sin comillas

Se contemplan formatos:

- `href="archivo.pdf"`
- `href='archivo.pdf'`
- `href=archivo.pdf`

Esto es crítico para índices de directorio estilo Apache.

### Normalización robusta

La normalización:

- Elimina fragmentos `#...`.
- Resuelve rutas relativas y jerárquicas (`..`, `.`).
- Conserva query string cuando corresponde.
- Preserva slash final de directorio (`/`) para resolver relativos correctamente.
- Filtra esquemas no navegables (`javascript:`, `mailto:`, `tel:`, `data:`).

### Restricción de dominio

Solo encola URLs cuyo host coincide con el host base.

Objetivo:

- Evitar descargar internet completo.
- Mantener el crawl acotado al sitio objetivo.

---

## Guardado local y reescritura de enlaces

### Estrategia de nombres y rutas

Cada URL canónica se mapea a un path local:

- Directorio -> `.../index.html`
- Query -> sufijo `__q_...`
- Caracteres no válidos -> sanitizados

Ejemplo:

- `https://sitio.com/docs/` -> `mirror/docs/index.html`
- `https://sitio.com/lista?C=N;O=D` -> `mirror/lista/index__q_C_N_O_D.html`

### Reescritura interna

Si el recurso es HTML/CSS:

- URLs del mismo host se reemplazan por rutas relativas locales.
- Esto permite navegar el mirror desde `localhost`.

---

## Ejemplos de ejecución

### 1) Sitio pequeño de prueba

```bash
./WebCrawler https://example.com mirror_example 4
```

### 2) Sitio real con más contenido

```bash
./WebCrawler https://site.com mirror_site 8
```

### 3) Directorio académico (índices Apache)

```bash
./WebCrawler http://148.204.58.221/axel/aplicaciones/diapositivas/ mirror_prof 6
```

### 4) Limitar duración con `timeout`

```bash
timeout 60 ./WebCrawler https://site.com mirror_site 8
```

---

## Verificación del mirror local

Levanta un servidor local en la carpeta mirror:

```bash
cd mirror_site
python3 -m http.server 8000
```

Abre:

- `http://localhost:8000/`
- o `http://127.0.0.1:8000/`

Si quieres que solo escuche en loopback:

```bash
python3 -m http.server 8000 --bind 127.0.0.1
```

---

## Problemas comunes y soluciones

### 1) "No se descargan PDFs/PPTs de un directorio"

Causas típicas:

- `href` sin comillas.
- Resolución de relativo incorrecta por perder slash final del directorio.

Estado actual:

- Ya corregido en esta práctica.

### 2) Se abre localhost pero parece el sitio oficial

Eso pasa cuando scripts o enlaces remotos redirigen fuera de local.
Revisa en DevTools > Network si hay requests al dominio original.

### 3) Ctrl+C no detiene

En otra terminal:

```bash
pgrep -af WebCrawler
kill <PID>
```

Si no termina:

```bash
kill -9 <PID>
```

### 4) El crawl tarda demasiado

- Reduce alcance usando URL más específica.
- Baja hilos si el servidor responde lento.
- Usa `timeout`.

---

## Limitaciones actuales

- No ejecuta JavaScript como un navegador real (sin motor headless).
- No cubre contenido que depende totalmente de llamadas dinámicas en runtime.
- Mantiene restricción a mismo host (diseño intencional).
- No implementa por ahora:
  - respeto de `robots.txt`
  - rate limiter fino por host
  - política de reintentos avanzada con backoff

---

## Mejoras sugeridas

1. Límite configurable de profundidad y/o cantidad máxima de recursos.
2. Reintentos por código HTTP/transiente con backoff exponencial.
3. Persistencia de estado para reanudar crawls interrumpidos.
4. Reporte final con métricas:
   - total descargado
   - errores por tipo
   - bytes transferidos
   - tiempo total
5. Modo verbose/debug configurable.
6. Integrar cola de prioridad (por tipo de recurso o por profundidad).

---

## Nota final

Esta práctica ya implementa un crawler robusto para escenarios reales, incluyendo índices de directorio y descarga de binarios enlazados. La base es sólida para evolucionar a un mirror todavía más completo.

