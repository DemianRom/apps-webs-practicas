# Practica 2 - Reproductor UDP en Python

## Contexto academico

Practica desarrollada para la unidad de aprendizaje **Aplicaciones para Comunicaciones en Web**, grupo **6CM3**, sexto semestre de la **Ingenieria en Sistemas Computacionales** en **ESCOM - IPN**.

## Integrantes

- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

## Descripcion

Este proyecto es un reproductor de musica sencillo hecho en Python. La comunicacion entre cliente y servidor se hace con sockets de datagrama (`UDP`), como pide la practica.

El servidor tiene canciones en la carpeta `canciones/`. El cliente pide la lista de canciones disponibles, selecciona una, la descarga completa usando control de flujo y, cuando termina de recibirse toda la informacion de la cancion, permite reproducirla.

La idea fue dejar una implementacion casera y clara, sin librerias raras. Se usan librerias normales de Python como `socket`, `tkinter`, `threading`, `queue`, `struct`, `json`, `pathlib` y `winsound`.

## Archivos

- `protocol.py`: contiene las constantes del proyecto y las funciones para armar y leer los paquetes que viajan por UDP. Aqui se definen los tipos de mensaje, el tamano de bloque, el tamano de ventana, el puerto, los ACK, los paquetes de datos, los errores y la senal de fin.
- `servidor.py`: levanta el servidor UDP. Atiende peticiones para listar canciones y para enviar una cancion por partes desde la carpeta `canciones/`. Usa sliding window: manda varios paquetes dentro de una ventana y avanza cuando recibe ACK acumulado.
- `cliente.py`: contiene la clase `MusicClient`, que pide la lista de canciones y descarga una cancion desde el servidor. Recibe paquetes UDP, guarda temporalmente los que lleguen dentro de la ventana y confirma el ultimo paquete consecutivo recibido. Tambien se puede ejecutar como cliente de consola para probar sin interfaz grafica.
- `gui.py`: interfaz grafica sencilla hecha con `tkinter`. Muestra las canciones del servidor, permite descargar la seleccionada, muestra el progreso y reproduce la cancion descargada.
- `README.md`: este documento. Explica como esta organizado el proyecto, como correrlo y como funciona el protocolo.
- `canciones/`: carpeta donde el servidor busca los archivos `.wav`.
- `descargadas/`: carpeta donde el cliente guarda las canciones recibidas.

## Como correrlo

Primero abre una terminal en esta carpeta y levanta el servidor:

```bash
python servidor.py
```

Luego abre otra terminal y corre la interfaz:

```bash
python gui.py
```

Tambien puedes probar sin interfaz:

```bash
python cliente.py
```

## Protocolo

El cliente manda `LIST` para pedir canciones o `GET nombre.wav` para descargar una cancion.

La descarga usa control de flujo con sliding window:

1. El servidor divide la cancion en paquetes numerados.
2. El servidor manda varios paquetes mientras quepan en la ventana.
3. El cliente recibe paquetes UDP y guarda los que pertenecen a su ventana de recepcion.
4. El cliente escribe en el archivo solo los paquetes que ya estan completos y en orden.
5. El cliente responde con un `ACK` acumulado, indicando el ultimo paquete consecutivo recibido.
6. El servidor mueve la ventana cuando recibe el `ACK`.
7. Si el servidor no recibe ACK a tiempo, reenvia la ventana desde el primer paquete pendiente.
8. Cuando termina, manda un paquete de fin.

La cancion solo se puede reproducir despues de que la descarga termino completa.

## Flujo general

1. Se ejecuta `servidor.py`.
2. El servidor abre un socket UDP en `127.0.0.1:5000`.
3. Se ejecuta `gui.py` o `cliente.py`.
4. El cliente pide la lista con `LIST`.
5. El servidor responde con los nombres y tamanos de las canciones `.wav`.
6. El usuario selecciona una cancion.
7. El cliente manda `GET nombre.wav`.
8. El servidor envia la cancion en bloques numerados usando ventana deslizante.
9. El cliente confirma con ACK acumulados.
10. Al terminar, el archivo queda guardado en `descargadas/`.
11. La interfaz habilita la reproduccion.

## Notas

- El reproductor integrado usa `winsound`, que viene con Python en Windows y reproduce archivos `.wav`.
- La practica esta pensada para ejecutarse localmente, por eso el host por defecto es `127.0.0.1`.
- Si se quiere usar entre dos computadoras de la misma red, se puede cambiar el `HOST` en `protocol.py` o al crear el cliente/servidor.
- Los archivos `.wav` de prueba no se incluyen en el repositorio para mantenerlo limpio. Para probar la practica, coloca una o mas canciones `.wav` dentro de `canciones/`; los archivos recibidos por el cliente se generaran en `descargadas/`.
