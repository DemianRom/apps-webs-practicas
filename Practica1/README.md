# Practica 1 - Servicio de transferencia de archivos

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Objetivo

Construir una aplicacion cliente-servidor capaz de administrar archivos remotos mediante sockets de flujo. La practica introduce el diseno de un protocolo sencillo: una conexion TCP para comandos y metadatos, y otra conexion TCP para transferir el contenido binario.

## Descripcion

El servidor esta implementado en C y el cliente en Java con interfaz Swing. Desde el cliente se pueden listar, subir, descargar, borrar y renombrar archivos o carpetas. La comunicacion de control usa JSON y la comunicacion de datos envia bytes crudos.

Puertos usados:

- `8000`: canal de metadatos y comandos JSON.
- `8001`: canal de datos para archivos.

## Estructura

```text
Practica1/
+-- Cliente/
|   +-- pom.xml
|   +-- README.md
|   +-- src/main/java/practica1/
|       +-- Cliente.java
|       +-- ClienteGUI.java
|       +-- TestGUI.java
+-- Servidor/
|   +-- Makefile
|   +-- servidor.c
|   +-- cJSON.c
|   +-- cJSON.h
|   +-- servidor_archivos/
+-- docs/
|   +-- Reporte_Practica_1_Redes_2.docx
|   +-- guia_practica1.pdf
+-- run_all.sh
+-- README.md
```

## Requisitos

- Java 21 o superior.
- Maven 3.x.
- GCC con soporte para sockets POSIX.
- Linux o WSL para compilar y ejecutar el servidor C.

## Ejecucion

Servidor:

```bash
cd Practica1/Servidor
make
./servidor
```

Cliente:

```bash
cd Practica1/Cliente
mvn compile
mvn exec:java -Dexec.mainClass="practica1.ClienteGUI"
```

## Operaciones soportadas

- Listar contenido local y remoto.
- Subir archivos.
- Descargar archivos.
- Subir carpetas.
- Descargar carpetas.
- Borrar archivos locales o remotos.
- Renombrar archivos y carpetas.

## Protocolo

Los comandos viajan como JSON de una sola linea por el puerto `8000`. El contenido de archivos viaja por el puerto `8001` como bytes crudos. El receptor lee exactamente el numero de bytes indicado en el campo `tamanio`.

Ejemplos:

```json
{"cmd":"LIST"}
{"cmd":"UPLOAD","nombre":"archivo.pdf","tamanio":204800}
{"cmd":"DOWNLOAD","nombre":"archivo.pdf"}
{"cmd":"DELETE","nombre":"archivo.pdf"}
{"cmd":"RENAME_FILE","actual":"a.txt","nuevo":"b.txt"}
```

## Aprendizajes

- Separar canal de control y canal de datos.
- Serializar metadatos con JSON.
- Coordinar cliente Java con servidor C.
- Manejar archivos y carpetas desde una aplicacion distribuida.
- Entender el valor de definir un protocolo claro antes de programar la interfaz.

## Valor dentro del semestre

Esta practica fue la base del curso. Nos obligo a pensar en comandos, respuestas, errores, tamanos y orden de operaciones. A partir de aqui las siguientes practicas pudieron crecer hacia UDP, concurrencia y aplicaciones mas interactivas.
