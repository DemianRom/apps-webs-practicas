# Aplicaciones para Comunicaciones en Red

Repositorio de practicas de la unidad de aprendizaje **Aplicaciones para Comunicaciones en Red**.

## Estructura

| Practica | Carpeta | Tema | Tecnologias | Estado |
|---|---|---|---|---|
| 1 | `Practica1/` | Servicio de transferencia de archivos con sockets de flujo | Java, C, TCP, JSON | Finalizada |
| 2 | `Practica2/` | Reproductor de musica con sockets de datagrama | Python, UDP, ventana deslizante | Finalizada |

## Organizacion general

Cada practica vive en su propia carpeta y contiene su propio README con instrucciones de ejecucion, estructura y descripcion tecnica. La raiz del repositorio se mantiene como indice general para agregar futuras practicas sin mezclar codigo, reportes ni recursos.

```text
redes2-repo-practicas/
+-- Practica1/
|   +-- Cliente/
|   +-- Servidor/
|   +-- docs/
|   +-- README.md
+-- Practica2/
|   +-- cliente.py
|   +-- servidor.py
|   +-- gui.py
|   +-- protocol.py
|   +-- README.md
+-- README.md
```

## Notas para nuevas practicas

- Crear una carpeta nueva con el formato `PracticaN/`.
- Agregar un `README.md` dentro de la carpeta de la practica.
- No subir archivos generados: compilados, caches, logs, carpetas `target/`, `__pycache__/`, ejecutables, objetos `.o` o descargas de prueba.
- Cuando una practica necesite archivos grandes de prueba, conservar solo la carpeta y documentar donde debe colocarlos el usuario.
