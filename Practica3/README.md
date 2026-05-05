# Practica 3 - Streaming MP3 con UDP, hilos y tuberia

## Contexto academico

Practica desarrollada para la unidad de aprendizaje **Aplicaciones para Comunicaciones en Web**, grupo **6CM3**, sexto semestre de la **Ingenieria en Sistemas Computacionales** en **ESCOM - IPN**.

## Integrantes

- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

## Descripcion

Esta practica parte de lo realizado en la Practica 2, donde ya existia una transferencia de canciones por sockets de datagrama (`UDP`) con control de flujo mediante ventana deslizante y ACK acumulados.

La diferencia principal de la Practica 3 es que ahora la logica de transferencia y la logica de reproduccion se separan en **dos hilos de ejecucion diferentes**:

- **Hilo de transferencia:** recibe la cancion por UDP, valida el orden de los paquetes, responde con ACK acumulado y guarda el archivo recibido.
- **Hilo de reproduccion:** consume los bytes de audio desde una tuberia y alimenta un archivo temporal de reproduccion.
- **Tuberia:** comunica ambos hilos mediante un buffer grande para evitar que la reproduccion se quede sin datos mientras la transferencia sigue llegando.

Tambien se cambio el formato de trabajo a **MP3** y se agrego extraccion de metainformacion desde el archivo: titulo, artista, album, anio y genero cuando el archivo contiene etiquetas ID3.

## Diferencias respecto a la Practica 2

| Aspecto | Practica 2 | Practica 3 |
|---|---|---|
| Formato de audio | WAV | MP3 |
| Reproduccion | Despues de descargar toda la cancion | Separada en otro hilo y alimentada por tuberia |
| Comunicacion interna | No usa tuberia | Usa `AudioPipe` con buffer grande |
| Metadatos | Solo nombre y tamano | Extrae ID3: artista, album, titulo, anio y genero |
| GUI | Descarga y luego reproduce | Muestra hilo de transferencia, hilo de reproduccion y estado del buffer |
| Red | UDP con ventana deslizante | Conserva UDP con ventana deslizante |

## Archivos

- `protocol.py`: constantes del protocolo, empaquetado de paquetes UDP, validacion de rutas MP3 y extraccion basica de metadatos ID3.
- `servidor.py`: servidor UDP que lista canciones `.mp3`, envia metadatos y transfiere archivos con ventana deslizante.
- `cliente.py`: cliente UDP, tuberia `AudioPipe`, hilo de transferencia y reproductor alimentado por tuberia.
- `gui.py`: interfaz grafica que muestra metadatos MP3, progreso UDP, estado de hilos y estado del buffer de la tuberia.
- `canciones/`: carpeta donde el servidor busca archivos `.mp3`.
- `descargadas/`: carpeta donde se guardan los MP3 recibidos y el archivo temporal de reproduccion.

## Como correrlo

Coloca una o mas canciones `.mp3` dentro de `canciones/`.

Primero levanta el servidor:

```bash
python servidor.py
```

Luego abre otra terminal y corre la interfaz:

```bash
python gui.py
```

Tambien se puede probar desde consola:

```bash
python cliente.py
```

## Protocolo de red

La practica conserva la base de la Practica 2:

1. El cliente manda `LIST` para pedir canciones.
2. El servidor responde con nombre, tamano y metadatos ID3 de cada MP3.
3. El cliente manda `GET nombre.mp3`.
4. El servidor envia un paquete de metadatos con el tamano total.
5. El servidor divide la cancion en paquetes UDP numerados.
6. El cliente acepta paquetes dentro de su ventana de recepcion.
7. El cliente confirma con ACK acumulados.
8. Si el servidor no recibe ACK a tiempo, reenvia desde la base de la ventana.
9. Al terminar, el servidor manda un paquete de fin.

## Tuberia entre hilos

La clase `AudioPipe` usa una cola bloqueante con capacidad amplia (`PIPE_MAX_CHUNKS`) para funcionar como tuberia entre el hilo que recibe paquetes UDP y el hilo que prepara la reproduccion. El buffer se mantiene suficientemente grande para reducir pausas durante la reproduccion.

El flujo interno es:

```text
Servidor UDP -> Hilo de transferencia -> AudioPipe -> Hilo de reproduccion -> Archivo temporal MP3
```

La GUI muestra el numero de chunks y bytes pendientes dentro de la tuberia, para que se pueda observar que transferencia y reproduccion no estan acopladas en una sola rutina.

## Notas

- Los archivos `.mp3` no se suben al repositorio para mantenerlo ligero.
- La reproduccion abre el archivo temporal con el reproductor predeterminado de Windows cuando ya existe un buffer inicial suficiente.
- Si el reproductor del sistema no reproduce archivos mientras siguen creciendo, la transferencia y la tuberia siguen funcionando; al finalizar queda el MP3 completo en `descargadas/`.
