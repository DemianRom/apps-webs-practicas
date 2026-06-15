# Cliente Java - Practica 1

Cliente grafico para el servicio de transferencia de archivos de la Practica 1.

## Ficha academica

- **Materia:** Aplicaciones y Comunicaciones en Red.
- **Profesor:** Axel Ernesto Moreno Cervantes.
- **Grupo:** 6CM1.
- **Periodo:** 26/2.
- **Alumnos:** Demian Romero Bautista y Said Ferreira Rodriguez.

## Descripcion

El cliente se conecta al servidor C mediante sockets TCP y usa dos canales:

- Puerto `8000`: canal permanente de metadatos en JSON.
- Puerto `8001`: canal intermitente de datos para transferir bytes de archivos.

La interfaz grafica esta implementada con Java Swing en `ClienteGUI.java`.

## Requisitos

- Java 21 o superior.
- Maven 3.x.

## Ejecucion

Antes de abrir el cliente, el servidor C debe estar ejecutandose desde `../Servidor`.

Desde esta carpeta:

```bash
mvn compile
mvn exec:java -Dexec.mainClass="practica1.ClienteGUI"
```

## Archivos principales

- `src/main/java/practica1/Cliente.java`: logica de red, protocolo JSON y operaciones de transferencia.
- `src/main/java/practica1/ClienteGUI.java`: interfaz grafica del cliente.
- `src/main/java/practica1/TestGUI.java`: clase auxiliar de prueba.
- `pom.xml`: configuracion Maven y dependencia Gson.
