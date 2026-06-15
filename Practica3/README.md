# Practica 3 - Streaming MP3 con UDP, hilos y tuberia

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Objetivo

Extender la transferencia UDP de la Practica 2 para permitir reproduccion durante la descarga. La practica separa la logica de red y la logica de audio en hilos distintos, comunicados por una tuberia sincronizada.

## Descripcion

El servidor envia canciones `.mp3` por UDP con ventana deslizante. El cliente recibe paquetes, confirma con ACK acumulados, guarda el archivo final y alimenta una tuberia (`AudioPipe`) para que otro hilo pueda reproducir mientras la transferencia sigue activa.

La interfaz muestra:

- Lista de canciones del servidor.
- Metadatos MP3.
- Progreso de descarga.
- Tiempo reproducido.
- Cache disponible.
- Estado de reproduccion.
- Canciones ya descargadas.

## Estructura

```text
Practica3/
+-- cliente.py
+-- servidor.py
+-- gui.py
+-- protocol.py
+-- requirements.txt
+-- canciones/
+-- descargadas/
+-- README.md
```

## Componentes principales

- `protocol.py`: formato de paquetes, ACK, metadatos MP3 y constantes del protocolo.
- `servidor.py`: servidor UDP con ventana deslizante y modo de demo lenta.
- `cliente.py`: cliente UDP, `AudioPipe`, sesiones de streaming y motor de reproduccion.
- `gui.py`: interfaz Tkinter para controlar transferencia y reproduccion.
- `requirements.txt`: dependencia `miniaudio`.

## Requisitos

- Python 3.10 o superior.
- Dependencias del proyecto:

```bash
pip install -r requirements.txt
```

## Ejecucion

Coloca canciones `.mp3` en `canciones/`.

Servidor:

```bash
cd Practica3
python servidor.py
```

GUI:

```bash
python gui.py
```

Cliente de consola:

```bash
python cliente.py
```

Modo demo lento:

```bash
python servidor.py --delay-ms 15
```

## Arquitectura de hilos

```text
Servidor UDP
    |
    v
Hilo de transferencia -> AudioPipe -> Hilo de reproduccion
    |
    v
queue.Queue -> GUI
```

El hilo de transferencia recibe paquetes, ordena datos y escribe en la tuberia. El hilo de reproduccion consume la tuberia con `miniaudio`. La GUI no toca sockets ni audio directamente; solo procesa eventos mediante una cola thread-safe.

## Sincronizacion

La practica usa primitivas seguras de Python:

- `threading.Lock`: protege el buffer compartido.
- `threading.Condition`: permite esperar datos sin hacer espera activa.
- `threading.Event`: cancela sesiones y coordina cierre.
- `queue.Queue`: comunica eventos hacia la GUI.

## Diferencias respecto a la Practica 2

| Aspecto | Practica 2 | Practica 3 |
|---|---|---|
| Formato | WAV | MP3 |
| Reproduccion | Despues de descargar | Durante la descarga |
| Concurrencia | Basica | Hilos coordinados |
| Comunicacion interna | No aplica | Tuberia de audio |
| Interfaz | Progreso simple | Progreso, cache, tiempo y descargadas |

## Pruebas recomendadas

```bash
python -m py_compile cliente.py gui.py protocol.py servidor.py
```

Despues:

1. Ejecuta `python servidor.py --delay-ms 15`.
2. Ejecuta `python gui.py`.
3. Selecciona una cancion grande.
4. Inicia la transferencia.
5. Verifica que la reproduccion pueda avanzar mientras se descarga.
6. Intenta adelantar a una zona no cargada y confirma que la GUI lo impide.
7. Espera al 100% y reproduce desde `descargadas/`.

## Aprendizajes

- Separar responsabilidades con hilos.
- Sincronizar productor y consumidor con una tuberia.
- Mantener una GUI responsiva mientras ocurren tareas de red y audio.
- Comprender el costo de reproducir datos que aun no han llegado.
- Reutilizar un protocolo UDP confiable para un caso mas exigente.

## Valor dentro del semestre

Esta practica fue una de las mas completas porque junto red, audio, concurrencia, GUI y validaciones de usuario. Nos permitio ver que una aplicacion de red real necesita coordinar muchas partes al mismo tiempo.
