# Aplicaciones para Comunicaciones en Red

Repositorio de prácticas de la materia **Aplicaciones para Comunicaciones en Red**.

##  Prácticas

| # | Título | Tema | Estado |
|---|--------|------|--------|
| 1 | Servicio de transferencia de archivos | Sockets de flujo (TCP) |  En progreso |

##  Tecnologías

- Java 21
- Maven
- `java.net` (Sockets)

##  Estructura del proyecto

```
redes2-repo-practicas/
└── src/main/java/
    └── practica1/   # Transferencia de archivos con sockets TCP
```

## ️ Cómo ejecutar

```bash
mvn compile
mvn exec:java -Dexec.mainClass="practica1.Cliente"
```

##  Requisitos

- Java 21+
- Maven 3.x