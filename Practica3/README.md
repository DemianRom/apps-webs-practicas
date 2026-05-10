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

## Sincronizacion de hilos

La practica si usa sincronizacion de hilos. No se hace con semaforos manuales, pero si con primitivas seguras de Python:

- `threading.Lock` dentro de `AudioPipe`: protege lectura/escritura concurrente del buffer compartido.
- `threading.Condition` dentro de `AudioPipe`: permite que el hilo reproductor espere datos cuando la tuberia esta vacia y despierte cuando el hilo de transferencia escribe.
- `threading.Event` en la sesion/cliente: permite cancelar transferencia y cierre ordenado.
- `queue.Queue` en GUI: paso de eventos thread-safe para actualizar interfaz sin tocar widgets desde hilos de trabajo.

Con esto se evita corrupcion del buffer, espera activa innecesaria y bloqueos por acceso concurrente a la GUI.

## Documentacion breve de funciones y clases

### `protocol.py`

- `pack_json(kind, payload)`: serializa mensaje JSON para lista/metadatos.
  Parametros: `kind` (`str`), `payload` (`dict`/`list`).
  Entrada: datos logicos de protocolo.
  Salida: `bytes`.
- `unpack_json(data)`: deserializa JSON recibido.
  Parametros: `data` (`bytes`).
  Entrada: paquete JSON.
  Salida: tupla `(kind, payload)`.
- `pack_meta(total_size)` / `unpack_meta(packet)`: empaqueta/desempaqueta tamano total de archivo.
  Parametros: `total_size` (`int`), `packet` (`bytes`).
  Salida: `bytes` o `int`.
- `pack_data(number, chunk)` / `unpack_data(packet)`: crea/lee paquete de datos numerado.
  Parametros: `number` (`int`), `chunk` (`bytes`).
  Salida: `bytes` o `(numero, bytes_chunk)`.
- `pack_ack(number)` / `unpack_ack(packet)`: crea/lee ACK acumulado.
  Parametros: `number` (`int`), `packet` (`bytes`).
  Salida: `bytes` o `int`.
- `pack_end()`: paquete de fin de transferencia.
  Salida: `bytes`.
- `pack_error(message)` / `unpack_error(packet)`: error de protocolo.
  Parametros: `message` (`str`), `packet` (`bytes`).
  Salida: `bytes` o `str`.
- `safe_song_path(folder, filename)`: valida ruta segura y extension `.mp3`.
  Parametros: `folder` (`Path|str`), `filename` (`str`).
  Salida: `Path` valido o excepcion.
- `read_id3v2(path)`, `read_id3v1(path)`, `mp3_metadata(path)`: extraen metadatos MP3.
  Parametros: `path` (`Path|str`).
  Salida: `dict` con titulo, artista, album, anio, genero y duracion aproximada.
- `list_mp3_files(folder)`: lista MP3 del servidor.
  Parametros: `folder` (`Path|str`).
  Salida: lista ordenada de `Path`.

### `servidor.py`

- `send_with_ack(sock, packet, address, expected_ack)`: envia paquete de control y espera ACK exacto.
  Parametros: socket UDP, `packet` (`bytes`), `address` (`(host,port)`), `expected_ack` (`int`).
  Salida: `bool` exito/fallo.
- `send_song(sock, address, filename, delay_seconds=0)`: flujo completo de envio de una cancion.
  Entrada: nombre MP3 solicitado.
  Proceso: valida, envia metadata, envia paquetes por ventana, cierra con `END`.
  Salida: sin retorno; emite datagramas y logs.
- `send_with_sliding_window(sock, packets, address, delay_seconds=0)`: algoritmo de ventana deslizante.
  Parametros: lista de paquetes ya numerados.
  Salida: `bool` (termino o cancelo por demasiados timeout).
- `send_song_list(sock, address)`: envia lista de canciones con metadatos.
  Salida: mensaje JSON por UDP.
- `run_server(host=HOST, port=PORT, delay_ms=0)`: bucle principal del servidor.
  Entrada: host/puerto opcionales y retardo de demo.
  Salida: servicio UDP en ejecucion.

### `cliente.py`

- `emit(callback, *args)`: helper para publicar eventos si hay callback de GUI.
  Salida: `None`.
- `class AudioPipe`: buffer compartido entre hilo de transferencia e hilo de reproduccion.
  Metodos clave:
  `write(data)`, `read(size)`, `close()`, `available_bytes()`, `seek(offset)`.
  Sincronizacion: usa lock + condition.
- `class AudioPipeSource(miniaudio.StreamableSource)`: adapta `AudioPipe` al motor de audio.
  Entrada: lectura por chunks.
  Salida: bytes para decodificador.
- `class StreamResult`: estructura de estado final de la transferencia/reproduccion.
- `class StreamSession`: agrupa hilos, pipe y estado de una sesion activa.
- `class MusicClient`: cliente UDP de lista y descarga.
  Metodos principales:
  `list_songs()`, `start_stream(...)`, `cancel_stream(...)`.
  Entrada: comandos GUI/consola.
  Salida: eventos de progreso y archivo en `descargadas/`.
- `class AudioPlayer`: control de reproduccion (play/pause/resume/stop/seek).
  Entrada: comandos del usuario.
  Salida: estado de reproduccion y tiempo reproducido.
- `class MiniaudioPipeEngine`: motor real que reproduce desde tuberia o archivo local.
  Entrada: fuente de audio (`AudioPipeSource` o archivo).
  Salida: audio en salida del sistema.
- `main()`: demo de cliente por consola.

### `gui.py`

- `class MusicPlayerGUI(tk.Tk)`: interfaz principal.
  Responsabilidad: presentar estados, iniciar/cancelar transferencia, controlar reproduccion, validar movimientos de barra y mostrar descargadas.
- Funciones/metodos GUI clave:
  `load_songs()`, `start_transfer()`, `cancel_transfer()`, `pause_playback()`, `resume_playback()`, `play_downloaded()`, `on_seek_changed()`, `process_events()`.
  Entrada: acciones del usuario y eventos asincronos.
  Salida: actualizacion visual y llamadas a `MusicClient`/`AudioPlayer`.

## Resumen tecnico para exposicion

La practica 3 mantiene UDP y ventana deslizante de la practica 2, pero agrega concurrencia real:

1. Hilo de transferencia: recibe paquetes UDP, confirma ACK acumulado, guarda archivo y escribe bytes en `AudioPipe`.
2. Hilo de reproduccion: consume bytes de `AudioPipe` con `miniaudio`, permitiendo escuchar mientras sigue la descarga.
3. Hilo GUI: solo interfaz y visualizacion; no bloquea red ni audio.

La sincronizacion se resuelve con `Lock`, `Condition`, `Event` y `Queue`, con lo cual la demostracion cumple la separacion solicitada por el profesor entre envio y reproduccion, comunicados por tuberia.
