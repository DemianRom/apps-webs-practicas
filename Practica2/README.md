# Practica 2 - Reproductor con transferencia UDP

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Objetivo

Implementar una aplicacion cliente-servidor que transfiera canciones usando sockets de datagrama (`UDP`) y un mecanismo propio de control de flujo basado en ventana deslizante y ACK acumulados.

## Descripcion

El servidor publica canciones desde la carpeta `canciones/`. El cliente solicita la lista de canciones, descarga una seleccion y la guarda en `descargadas/`. La GUI en Tkinter muestra las canciones disponibles, el progreso de descarga y permite reproducir el archivo una vez recibido completo.

Aunque UDP no garantiza entrega ni orden, la practica agrega confiabilidad basica mediante:

- Paquetes numerados.
- Ventana deslizante.
- ACK acumulados.
- Reenvio por timeout.
- Paquete de fin de transferencia.

## Estructura

```text
Practica2/
+-- cliente.py
+-- servidor.py
+-- gui.py
+-- protocol.py
+-- canciones/
+-- descargadas/
+-- README.md
```

## Archivos principales

- `protocol.py`: constantes, formato de paquetes, tipos de mensaje y helpers del protocolo.
- `servidor.py`: servidor UDP que lista canciones y envia archivos por paquetes.
- `cliente.py`: cliente de consola y clase `MusicClient` para pedir listas y descargar canciones.
- `gui.py`: interfaz grafica con Tkinter.
- `canciones/`: carpeta de entrada del servidor.
- `descargadas/`: carpeta donde el cliente guarda lo recibido.

## Requisitos

- Python 3.10 o superior.
- Windows para reproduccion con `winsound`, o adaptacion del reproductor si se usa otro sistema.

## Ejecucion

Servidor:

```bash
cd Practica2
python servidor.py
```

Interfaz grafica:

```bash
python gui.py
```

Cliente de consola:

```bash
python cliente.py
```

## Flujo de transferencia

1. El cliente envia `LIST`.
2. El servidor responde con canciones disponibles.
3. El cliente envia `GET nombre`.
4. El servidor divide el archivo en paquetes numerados.
5. El servidor envia varios paquetes dentro de una ventana.
6. El cliente guarda paquetes recibidos y confirma el ultimo consecutivo.
7. El servidor avanza la ventana cuando recibe ACK.
8. Si hay timeout, se reenvia desde el primer paquete pendiente.
9. Al terminar, el cliente guarda la cancion y habilita la reproduccion.

## Aprendizajes

- Comprender que UDP es rapido, pero no confiable por si mismo.
- Construir confiabilidad basica desde la capa de aplicacion.
- Manejar paquetes, orden, perdidas y reenvios.
- Integrar red, archivos e interfaz grafica.
- Visualizar la diferencia entre transferencia completa y reproduccion posterior.

## Valor dentro del semestre

Esta practica conecto la teoria de protocolos con una experiencia visible: descargar una cancion por una red no confiable. Fue el puente entre sockets simples y disenos mas complejos de streaming.
