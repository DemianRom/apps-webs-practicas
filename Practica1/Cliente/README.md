# Cliente Java - Practica 1

Cliente para el servicio de transferencia de archivos de la Practica 1.

## Contexto academico

Este modulo forma parte de la unidad de aprendizaje **Aplicaciones para Comunicaciones en Web**, grupo **6CM3**, sexto semestre de la **Ingenieria en Sistemas Computacionales** en **ESCOM - IPN**.

## Integrantes

- Romero Bautista Demian
- Ferreira Rodriguez Hector Said
- Jaimes Uribe Mateo Alejandro

## Descripcion

El cliente se conecta al servidor C mediante sockets TCP. Usa dos canales:

- Puerto `8000`: canal permanente de metadatos en JSON.
- Puerto `8001`: canal intermitente de datos para transferir bytes de archivos.

La interfaz grafica esta implementada con Java Swing en `ClienteGUI.java`.

## Requisitos

- Java 21 o superior.
- Maven 3.x.

## Ejecucion

Desde esta carpeta:

```bash
mvn compile
mvn exec:java -Dexec.mainClass="practica1.ClienteGUI"
```

Antes de abrir el cliente, el servidor C debe estar ejecutandose desde `../Servidor`.

## Archivos principales

- `src/main/java/practica1/Cliente.java`: logica de red, protocolo JSON y operaciones de transferencia.
- `src/main/java/practica1/ClienteGUI.java`: interfaz grafica del cliente.
- `src/main/java/practica1/TestGUI.java`: clase auxiliar de prueba.
- `pom.xml`: configuracion Maven y dependencia Gson.
