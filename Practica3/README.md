# Practica 3 - Streaming MP3 con UDP, hilos y tuberia

## Contexto academico

Practica desarrollada para la unidad de aprendizaje **Aplicaciones para Comunicaciones en Web**, grupo **6CM3**, sexto semestre de la **Ingenieria en Sistemas Computacionales** en **ESCOM - IPN**.

## Integrantes

- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

## Objetivo

Esta practica parte de la Practica 2, donde ya se transferian canciones con sockets de datagrama (`UDP`) y control de flujo por ventana deslizante con ACK acumulados. La mejora de la Practica 3 consiste en separar la logica de transferencia y la logica de reproduccion en hilos diferentes, comunicados por una tuberia.

El resultado es un cliente que puede recibir una cancion por UDP mientras otro hilo va preparando su reproduccion. La interfaz muestra el avance de la descarga, el estado del hilo reproductor, el tiempo reproducido y el estado de la tuberia/cache.

## Que hace la practica

- Lista canciones `.mp3` disponibles en el servidor.
- Extrae metadatos MP3: titulo, artista, album, anio, genero y duracion aproximada.
- Descarga canciones por UDP usando paquetes numerados.
- Usa ACK acumulados para confirmar hasta que paquete fue recibido correctamente.
- Usa una ventana deslizante para mantener control de flujo sobre UDP.
- Ejecuta la transferencia y la reproduccion en hilos separados.
- Comunica ambos hilos con `AudioPipe`, una tuberia con cache interno.
- Muestra progreso de descarga, segundos reproducidos y cache disponible en MB.
- Permite pausar y reanudar la reproduccion.
- Permite mover la barra de reproduccion solo dentro de lo ya cargado.
- Valida cuando el usuario intenta adelantar a una zona aun no descargada.
- Guarda las canciones completas en `descargadas/`.
- Permite reproducir archivos ya descargados desde la GUI.
- Incluye modo de demostracion lenta para que se vea la diferencia entre descargar y reproducir.

## Diferencias respecto a la Practica 2

| Aspecto | Practica 2 | Practica 3 |
|---|---|---|
| Transporte | UDP con control de flujo | Conserva UDP con control de flujo |
| Audio | WAV | MP3 |
| Reproduccion | Despues de descargar | En hilo separado |
| Comunicacion interna | Sin tuberia | `AudioPipe` entre descarga y reproduccion |
| GUI | Progreso basico | Progreso, buffer, segundos y descargadas |
| Metadatos | Nombre/tamano | ID3 y duracion aproximada |

## Componentes principales

### `protocol.py`

Define constantes y funciones del protocolo:

- `CHUNK_SIZE`: tamano de bloque enviado por UDP.
- `WINDOW_SIZE`: cantidad maxima de paquetes sin confirmar.
- `PIPE_START_BYTES`: cantidad minima de bytes antes de iniciar reproduccion.
- `pack_data`, `unpack_data`, `pack_ack`, `pack_meta`: empaquetado binario.
- `mp3_metadata`: extraccion de metadatos ID3 y duracion.

Se usa para mantener el contrato de comunicacion separado del cliente y del servidor.

### `servidor.py`

Implementa el servidor UDP. Sus tareas son:

- Escuchar solicitudes `LIST` y `GET`.
- Enviar la lista de canciones con metadatos.
- Dividir cada MP3 en paquetes numerados.
- Aplicar ventana deslizante.
- Esperar ACK acumulados del cliente.
- Reenviar paquetes si se pierde confirmacion.
- Permitir `--delay-ms` para hacer lenta la demo.

Se mantiene UDP porque la Practica 2 ya trabajaba con datagramas y control de flujo manual. La ventana y los ACK demuestran como construir confiabilidad basica sobre UDP.

### `cliente.py`

Contiene la logica de red y reproduccion:

- `MusicClient`: lista y descarga canciones por UDP.
- `StreamSession`: agrupa resultado, hilo de transferencia, hilo de reproduccion y tuberia.
- `AudioPipe`: tuberia/cache entre los hilos.
- `AudioPipeSource`: fuente de bytes consumida por `miniaudio`.
- `AudioPlayer`: controla reproduccion, pausa, reanudacion y cierre.
- `MiniaudioPipeEngine`: reproduce MP3 desde tuberia o desde archivo descargado.

La separacion permite explicar el flujo sin mezclar GUI, sockets y audio en la misma clase.

### `gui.py`

Implementa la interfaz grafica. La GUI no lee sockets ni reproduce audio directamente. Solo:

- Inicia/cancela sesiones.
- Muestra canciones disponibles.
- Muestra canciones descargadas.
- Actualiza barras y etiquetas.
- Envia comandos de pausa, reanudacion y movimiento de barra.

Los hilos reportan eventos a la interfaz mediante `queue.Queue`. Esta cola funciona como una segunda tuberia, pero solo para informacion visual.

### `requirements.txt`

Incluye la dependencia de audio:

```bash
miniaudio
```

`miniaudio` se usa para reproducir MP3 dentro de Python sin abrir un reproductor externo.

## Arquitectura de hilos

La practica usa dos hilos principales por sesion de streaming:

```text
Hilo de transferencia
Servidor UDP -> paquetes numerados -> ordena -> guarda archivo -> escribe en AudioPipe

Hilo de reproduccion
AudioPipe -> decodificador Miniaudio -> salida de audio
```

La GUI recibe eventos por una cola:

```text
Hilos de trabajo -> queue.Queue -> GUI
```

Esto permite mostrar al profesor dos comunicaciones internas:

- **Tuberia de audio:** mueve bytes de la descarga hacia la reproduccion.
- **Tuberia de eventos:** mueve estados hacia la interfaz.

## Por que se usa `AudioPipe`

La tuberia evita acoplar directamente la velocidad de descarga con la velocidad de reproduccion. La red puede descargar mas rapido que el audio, o el audio puede quedarse esperando si la red va lenta.

`AudioPipe` conserva un cache interno con los bytes recibidos. El buffer visible se calcula como:

```text
bytes descargados - bytes reproducidos
```

Esto permite mostrar cuantos MB estan disponibles para el reproductor.

## Movimiento de la barra de reproduccion

La barra de reproduccion esta validada contra lo que ya se cargo. La GUI calcula hasta que segundo se ha descargado usando:

```text
segundos cargados = duracion total * bytes recibidos / tamano total
```

Si el usuario intenta adelantar a una zona que aun no ha llegado por UDP, se muestra una alerta y la barra regresa a la posicion actual. Si el usuario retrocede o se mueve dentro de lo ya cargado, se permite el movimiento y la descarga sigue ejecutandose en su hilo.

Esta validacion ayuda a demostrar que no se puede reproducir una parte de la cancion que todavia no existe en el cliente.

## Canciones descargadas

Cuando una transferencia llega al 100%, el archivo se guarda en:

```text
descargadas/
```

La GUI actualiza el menu **Descargadas** y permite reproducir la cancion como archivo local completo. Esta reproduccion ya no depende del hilo de transferencia ni del servidor.

## Como correrlo

Instala dependencias:

```bash
pip install -r requirements.txt
```

Coloca canciones `.mp3` en:

```text
canciones/
```

Levanta el servidor:

```bash
python servidor.py
```

Abre la GUI:

```bash
python gui.py
```

Tambien se puede probar por consola:

```bash
python cliente.py
```

## Modo demo lento

Para que se vea claramente que la reproduccion puede avanzar mientras la descarga sigue, ejecuta el servidor con retraso artificial:

```bash
python servidor.py --delay-ms 8
```

Si quieres que la transferencia tarde mas:

```bash
python servidor.py --delay-ms 15
```

Esto no cambia el protocolo. Solo agrega una pausa por paquete UDP para facilitar la demostracion.

## Pruebas recomendadas

Compilar todos los archivos:

```bash
python -m py_compile cliente.py gui.py protocol.py servidor.py
```

Probar transferencia:

1. Ejecutar `python servidor.py --delay-ms 15`.
2. Ejecutar `python gui.py`.
3. Seleccionar una cancion grande.
4. Presionar **Iniciar transferencia UDP**.
5. Verificar que suba el porcentaje de descarga.
6. Verificar que aparezca cache disponible.
7. Intentar mover la barra mas alla de **Cargado hasta** y confirmar que se muestra alerta.
8. Esperar al 100% y reproducir desde **Descargadas**.

## Notas

- Los archivos `.mp3` no se suben al repositorio para mantenerlo ligero.
- La carpeta `descargadas/` se usa para guardar archivos recibidos durante las pruebas.
- La cancelacion cierra la tuberia y detiene los hilos de la sesion.
- La practica conserva UDP y control de flujo de la Practica 2, pero agrega concurrencia y tuberia para cumplir la Practica 3.
