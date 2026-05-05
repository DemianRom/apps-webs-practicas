# Practica 1 - Servicio de transferencia de archivos

## Descripcion

Implementacion de un servicio cliente-servidor para transferir archivos y carpetas mediante sockets de flujo bloqueantes. El cliente esta desarrollado en Java y el servidor en C.

La practica usa dos conexiones TCP:

- **Puerto 8000 - metadatos:** conexion permanente para comandos y respuestas JSON.
- **Puerto 8001 - datos:** conexion intermitente para enviar o recibir bytes crudos de archivos.

## Integrantes

| Nombre | Responsabilidad |
|---|---|
| Demian Romero Bautista | Cliente Java y protocolo de red |
| Said Ferreira | Servidor C |
| Mateo Alejandro Jaimes Uribe | Interfaz grafica Swing |

## Estructura

```text
Practica1/
+-- Cliente/
|   +-- pom.xml
|   +-- src/main/java/practica1/
|       +-- Cliente.java
|       +-- ClienteGUI.java
|       +-- TestGUI.java
+-- Servidor/
|   +-- Makefile
|   +-- servidor.c
|   +-- cJSON.c
|   +-- cJSON.h
+-- docs/
|   +-- Reporte_Practica_1_Redes_2.docx
|   +-- guia_practica1.pdf
+-- run_all.sh
```

## Requisitos

- Java 21 o superior.
- Maven 3.x.
- GCC con soporte para sockets POSIX.
- En Windows, el servidor C se puede compilar/ejecutar desde WSL.

## Ejecucion

### Servidor

```bash
cd Practica1/Servidor
make
./servidor
```

### Cliente

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
- Renombrar archivos y carpetas locales o remotos.

## Protocolo

Los metadatos viajan como JSON de una sola linea por el puerto 8000. El contenido de archivos viaja por el puerto 8001 como bytes crudos. El receptor lee exactamente la cantidad de bytes indicada en el campo `tamanio`.

Ejemplos de comandos:

```json
{"cmd":"LIST"}
{"cmd":"UPLOAD","nombre":"archivo.pdf","tamanio":204800}
{"cmd":"DOWNLOAD","nombre":"archivo.pdf"}
{"cmd":"DELETE","nombre":"archivo.pdf"}
{"cmd":"RENAME_FILE","actual":"a.txt","nuevo":"b.txt"}
```
