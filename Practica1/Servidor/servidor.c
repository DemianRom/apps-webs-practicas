/*
 * Practica 1 - Servicio de transferencia de archivos
 * Materia: Aplicaciones y Comunicaciones en Red (6CM1)
 * ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
 * Periodo: 26/2
 *
 * Integrantes:
 * - Romero Bautista Demian
 * - Ferreira Rodriguez Said
 *
 * Responsabilidad del archivo:
 * Implementa el servidor TCP en C, procesa comandos JSON y administra operaciones sobre archivos remotos.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "cJSON.h"

#define PORT_META 8000
#define PORT_DATA 8001
#define BUFFER_SIZE 4096
#define DIR_ARCHIVOS "servidor_archivos"

/*
 * =====================================================================
 *  Servidor de Transferencia de Archivos (C)
 *  ---------------------------------------------------------------
 *  Arquitectura: DOS CANALES (METADATOS + DATOS)
 *
 *  - Canal de metadatos: puerto 8000 (constante PORT_META)
 *    · Permanente por cliente: se hace accept() y se lee línea a línea
 *      (JSON por línea, delimitado por '\n').
 *    · Aquí se procesan comandos JSON como LIST, UPLOAD, DOWNLOAD, etc.
 *
 *  - Canal de datos: puerto 8001 (constante PORT_DATA)
 *    · Intermitente: el servidor escucha y hace accept() cada vez
 *      que necesita transferir bytes crudos (tanto para subir como
 *      para descargar). Transporta exactamente N bytes, no texto.
 *
 *  Contrato JSON (resumen):
 *    - Cliente -> Servidor: siempre envía primero un objeto JSON
 *      en una sola línea y espera una respuesta (protocolo pregunta-respuesta).
 *    - Mensajes de ejemplo:
 *        {"cmd":"LIST"}
 *        {"cmd":"UPLOAD","nombre":"foo.txt","tamanio":12345}
 *        {"cmd":"DOWNLOAD","nombre":"bar.bin"}
 *        {"cmd":"UPLOAD_DIR","nombre":"fotos","cantidad":3}
 *        {"cmd":"FILE","nombre":"fotos/img.jpg","tamanio":81920}
 *        {"cmd":"EXIT"}
 *    - Respuestas (servidor -> cliente):
 *        {"status":"OK"}
 *        {"status":"ITEM","tipo":"FILE|DIR","nombre":"x","tamanio":N}
 *        {"status":"FIN_LIST"}
 *        {"status":"ERROR","msg":"..."}
 *        {"status":"UPLOAD_OK"}
 *        {"status":"NEXT","nombre":"x","tamanio":N}
 *
 *  Implementación (alto nivel):
 *    - main(): crea sockets de metadatos y datos, hace listen() en ambos.
 *    - Bucle principal: accept() en socket de metadatos; por cada cliente
 *      se llama a handle_client(client_fd, data_server_fd).
 *    - handle_client(): envuelve el socket en FILE* con fdopen(dup()) y
 *      lee líneas con fgets(). Por cada línea invoca process_command().
 *    - process_command(): parsea el JSON y ejecuta la operación correspondiente.
 *      Para transferencias de bytes hace accept() en el socket de datos y
 *      realiza recv()/send() para leer o escribir exactamente el número
 *      de bytes indicado por el cliente.
 *
 *  Recomendaciones importantes:
 *    - Validar/sanear el campo "nombre" para evitar path traversal.
 *    - Evitar bloqueos prolongados: agregar timeouts y límites de tamaño.
 *    - Para concurrencia, crear hilos (pthread_create) o procesos por cliente.
 *
 * =====================================================================
 */

// prototipos
/*
 * handle_client
 * ----------------
 * Atiende la conexión de metadatos de un cliente. Lee líneas (JSON
 * terminadas en '\n') desde el socket, y llama a process_command
 * para cada mensaje. El parámetro data_server_fd es el descriptor del
 * socket de escucha del canal de datos (PORT_DATA) y se pasa a
 * process_command para que éste haga accept() cuando necesite la
 * transferencia de bytes.
 */
void handle_client(int client_socket, int data_server_fd);

/*
 * process_command
 * -----------------
 * Procesa una línea JSON recibida por el canal de metadatos. El
 * parámetro 'stream' es el FILE* derivado del socket de metadatos
 * (usado en algunos flujos para leer acknowledgements cuando aplica).
 */
void process_command(int client_socket, int data_server_fd, const char *json_str, FILE *stream);

/*
 * send_json
 * ---------
 * Envía el objeto cJSON serializado seguido de '\n' para que el
 * cliente Java (BufferedReader.readLine()) pueda parsear por línea.
 */
void send_json(int socket, cJSON *json);

/*
 * send_error / send_ok
 * ---------------------
 * Utilidades para enviar respuestas estándar de error u OK en
 * formato JSON por el canal de metadatos.
 */
void send_error(int socket, const char *msg);
void send_ok(int socket);

int main() {
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Crear direcorio para archivos si no existe
    struct stat st = {0};
    if (stat(DIR_ARCHIVOS, &st) == -1) {
        mkdir(DIR_ARCHIVOS, 0700);
    }

    // Crear socket file descriptor para Metadatos
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket meta failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt meta");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT_META);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind meta failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0) {
        perror("listen meta");
        exit(EXIT_FAILURE);
    }

    // Crear socket file descriptor para Datos
    int data_server_fd;
    if ((data_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket data failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(data_server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt data");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in data_address;
    data_address.sin_family = AF_INET;
    data_address.sin_addr.s_addr = INADDR_ANY;
    data_address.sin_port = htons(PORT_DATA);

    if (bind(data_server_fd, (struct sockaddr *)&data_address, sizeof(data_address)) < 0) {
        perror("bind data failed");
        exit(EXIT_FAILURE);
    }
    if (listen(data_server_fd, 10) < 0) {
        perror("listen data");
        exit(EXIT_FAILURE);
    }

    printf("Servidor de metadatos escuchando en el puerto %d\n", PORT_META);
    printf("Servidor de datos escuchando en el puerto %d\n", PORT_DATA);

    while (1) {
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            continue;
        }

        printf("Nuevo cliente conectado (Metadatos)\n");

        handle_client(client_fd, data_server_fd);
    }

    return 0;
}

void handle_client(int client_socket, int data_server_fd) {

    char buffer[BUFFER_SIZE];
    FILE *stream = fdopen(dup(client_socket), "r");
    if (!stream) {
        perror("fdopen fallback");
        close(client_socket);
        return;
    }

    while (fgets(buffer, BUFFER_SIZE, stream) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;
        buffer[strcspn(buffer, "\r")] = 0;

        if (strlen(buffer) == 0) continue;

        printf("Recibido: %s\n", buffer);
        process_command(client_socket, data_server_fd, buffer, stream);
    }

    fclose(stream);
    close(client_socket);
    printf("Cliente desconectado.\n");
}

void process_command(int client_socket, int data_server_fd, const char *json_str, FILE *stream) {
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        send_error(client_socket, "Invalid JSON");
        return;
    }

    cJSON *cmd_item = cJSON_GetObjectItemCaseSensitive(json, "cmd");
    if (!cJSON_IsString(cmd_item) || (cmd_item->valuestring == NULL)) {
        send_error(client_socket, "Command missing or invalid");
        cJSON_Delete(json);
        return;
    }

    const char *cmd = cmd_item->valuestring;

    if (strcmp(cmd, "LIST") == 0) {
        DIR *d;
        struct dirent *dir;
        d = opendir(DIR_ARCHIVOS);
        if (d) {
            while ((dir = readdir(d)) != NULL) {
                if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;

                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", DIR_ARCHIVOS, dir->d_name);
                struct stat st;
                if (stat(filepath, &st) == 0) {
                    cJSON *item = cJSON_CreateObject();
                    cJSON_AddStringToObject(item, "status", "ITEM");
                    cJSON_AddStringToObject(item, "nombre", dir->d_name);

                    if (S_ISDIR(st.st_mode)) {
                        cJSON_AddStringToObject(item, "tipo", "DIR");
                        cJSON_AddNumberToObject(item, "tamanio", 0);
                    } else {
                        cJSON_AddStringToObject(item, "tipo", "FILE");
                        cJSON_AddNumberToObject(item, "tamanio", st.st_size);
                    }
                    send_json(client_socket, item);
                    cJSON_Delete(item);
                }
            }
            closedir(d);
        }

        cJSON *fin = cJSON_CreateObject();
        cJSON_AddStringToObject(fin, "status", "FIN_LIST");
        send_json(client_socket, fin);
        cJSON_Delete(fin);

    } else if (strcmp(cmd, "UPLOAD") == 0 || strcmp(cmd, "FILE") == 0) {
        cJSON *nombre_item = cJSON_GetObjectItemCaseSensitive(json, "nombre");
        cJSON *tamanio_item = cJSON_GetObjectItemCaseSensitive(json, "tamanio");
        if (cJSON_IsString(nombre_item) && cJSON_IsNumber(tamanio_item)) {
            const char *nombre = nombre_item->valuestring;
            long tamanio = (long)tamanio_item->valuedouble;

            // env OK
            send_ok(client_socket);

            // WEsperar conexion de datos
            struct sockaddr_in data_addr;
            socklen_t data_addrlen = sizeof(data_addr);
            int data_client = accept(data_server_fd, (struct sockaddr *)&data_addr, &data_addrlen);
            if (data_client >= 0) {
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", DIR_ARCHIVOS, nombre);

                FILE *fp = fopen(filepath, "wb");
                if (fp) {
                    char buf[BUFFER_SIZE];
                    long remaining = tamanio;
                    while (remaining > 0) {
                        size_t to_read = remaining < BUFFER_SIZE ? remaining : BUFFER_SIZE;
                        ssize_t n = recv(data_client, buf, to_read, 0);
                        if (n <= 0) break;
                        fwrite(buf, 1, n, fp);
                        remaining -= n;
                    }
                    fclose(fp);

                    cJSON *ok = cJSON_CreateObject();
                    if (strcmp(cmd, "UPLOAD") == 0) {
                        cJSON_AddStringToObject(ok, "status", "UPLOAD_OK");
                    } else {
                        cJSON_AddStringToObject(ok, "status", "FILE_OK");
                    }
                    send_json(client_socket, ok);
                    cJSON_Delete(ok);
                } else {
                    send_error(client_socket, "No se pudo crear el archivo");
                }
                close(data_client);
            } else {
                perror("accept data client");
                send_error(client_socket, "Error en conexion de datos");
            }
        } else {
            send_error(client_socket, "Faltan parametros");
        }
    } else if (strcmp(cmd, "DOWNLOAD") == 0) {
        cJSON *nombre_item = cJSON_GetObjectItemCaseSensitive(json, "nombre");
        if (cJSON_IsString(nombre_item)) {
            const char *nombre = nombre_item->valuestring;
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", DIR_ARCHIVOS, nombre);

            struct stat st;
            if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
                cJSON *resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "status", "OK");
                cJSON_AddNumberToObject(resp, "tamanio", st.st_size);
                send_json(client_socket, resp);
                cJSON_Delete(resp);

                struct sockaddr_in data_addr;
                socklen_t data_addrlen = sizeof(data_addr);
                int data_client = accept(data_server_fd, (struct sockaddr *)&data_addr, &data_addrlen);
                if (data_client >= 0) {
                    FILE *fp = fopen(filepath, "rb");
                    if (fp) {
                        char buf[BUFFER_SIZE];
                        size_t n;
                        while ((n = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
                            send(data_client, buf, n, 0);
                        }
                        fclose(fp);
                    }
                    close(data_client);
                }
            } else {
                send_error(client_socket, "Archivo no encontrado");
            }
        } else {
            send_error(client_socket, "Falta parametro nombre");
        }
    } else if (strcmp(cmd, "DOWNLOAD_OK") == 0) {
        printf("Descarga confirmada por el cliente.\n");
    } else if (strcmp(cmd, "EXIT") == 0) {

    } else if (strcmp(cmd, "DELETE") == 0) {
        cJSON *nombre_item = cJSON_GetObjectItemCaseSensitive(json, "nombre");
        if (cJSON_IsString(nombre_item)) {
            char filepath[1024];
            snprintf(filepath, sizeof(filepath), "%s/%s", DIR_ARCHIVOS, nombre_item->valuestring);
            if (remove(filepath) == 0) {
                cJSON *resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "status", "DELETE_OK");
                send_json(client_socket, resp);
                cJSON_Delete(resp);
            } else {
                send_error(client_socket, "No se pudo borrar");
            }
        } else {
            send_error(client_socket, "Faltan parametros");
        }
    } else if (strcmp(cmd, "RENAME_FILE") == 0 || strcmp(cmd, "RENAME_DIR") == 0) {
        cJSON *actual_item = cJSON_GetObjectItemCaseSensitive(json, "actual");
        cJSON *nuevo_item = cJSON_GetObjectItemCaseSensitive(json, "nuevo");
        if (cJSON_IsString(actual_item) && cJSON_IsString(nuevo_item)) {
            char path_actual[1024], path_nuevo[1024];
            snprintf(path_actual, sizeof(path_actual), "%s/%s", DIR_ARCHIVOS, actual_item->valuestring);
            snprintf(path_nuevo, sizeof(path_nuevo), "%s/%s", DIR_ARCHIVOS, nuevo_item->valuestring);
            if (rename(path_actual, path_nuevo) == 0) {
                cJSON *resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "status", "RENAME_OK");
                send_json(client_socket, resp);
                cJSON_Delete(resp);
            } else {
                send_error(client_socket, "No se pudo renombrar");
            }
        } else {
            send_error(client_socket, "Faltan parametros");
        }
    } else if (strcmp(cmd, "UPLOAD_DIR") == 0) {
        cJSON *nombre_item = cJSON_GetObjectItemCaseSensitive(json, "nombre");
        if (cJSON_IsString(nombre_item)) {
            char dirpath[1024];
            snprintf(dirpath, sizeof(dirpath), "%s/%s", DIR_ARCHIVOS, nombre_item->valuestring);
            mkdir(dirpath, 0700);
            send_ok(client_socket);
        } else {
            send_error(client_socket, "Faltan parametros");
        }
    } else if (strcmp(cmd, "DOWNLOAD_DIR") == 0) {
        cJSON *nombre_item = cJSON_GetObjectItemCaseSensitive(json, "nombre");
        if (cJSON_IsString(nombre_item)) {
            char dirpath[1024];
            snprintf(dirpath, sizeof(dirpath), "%s/%s", DIR_ARCHIVOS, nombre_item->valuestring);

            DIR *d = opendir(dirpath);
            if (d) {
                // Count files
                int count = 0;
                struct dirent *dir;
                while ((dir = readdir(d)) != NULL) {
                    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
                    char filepath[1024];
                    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dir->d_name);
                    struct stat st;
                    if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
                        count++;
                    }
                }
                rewinddir(d);

                cJSON *resp = cJSON_CreateObject();
                cJSON_AddStringToObject(resp, "status", "OK");
                cJSON_AddNumberToObject(resp, "cantidad", count);
                send_json(client_socket, resp);
                cJSON_Delete(resp);

                while ((dir = readdir(d)) != NULL) {
                    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) continue;
                    char filepath[1024];
                    snprintf(filepath, sizeof(filepath), "%s/%s", dirpath, dir->d_name);
                    struct stat st;
                    if (stat(filepath, &st) == 0 && S_ISREG(st.st_mode)) {
                        cJSON *next_msg = cJSON_CreateObject();
                        cJSON_AddStringToObject(next_msg, "status", "NEXT");
                        cJSON_AddStringToObject(next_msg, "nombre", dir->d_name);
                        cJSON_AddNumberToObject(next_msg, "tamanio", st.st_size);
                        send_json(client_socket, next_msg);
                        cJSON_Delete(next_msg);

                        struct sockaddr_in data_addr;
                        socklen_t data_addrlen = sizeof(data_addr);
                        int data_client = accept(data_server_fd, (struct sockaddr *)&data_addr, &data_addrlen);
                        if (data_client >= 0) {
                            FILE *fp = fopen(filepath, "rb");
                            if (fp) {
                                char buf[BUFFER_SIZE];
                                size_t n;
                                while ((n = fread(buf, 1, BUFFER_SIZE, fp)) > 0) {
                                    send(data_client, buf, n, 0);
                                }
                                fclose(fp);
                            }
                            close(data_client);

                            // Wait for NEXT_OK
                            char ok_buf[BUFFER_SIZE];
                            if (fgets(ok_buf, BUFFER_SIZE, stream) != NULL) {
                                // Assume it's NEXT_OK, could parse it for robustness
                            }
                        }
                    }
                }
                closedir(d);
            } else {
                send_error(client_socket, "Carpeta no encontrada");
            }
        } else {
            send_error(client_socket, "Faltan parametros");
        }
    } else {
        send_error(client_socket, "Unknown command");
    }

    cJSON_Delete(json);
}

void send_json(int socket, cJSON *json) {
    char *string = cJSON_PrintUnformatted(json);
    if (string == NULL) {
        fprintf(stderr, "Failed to print json\n");
        return;
    }

    // printf("Enviando: %s\n", string);

    // AAgregar salto de linea para que el cliente sepa que es el final del mensaje
    size_t len = strlen(string);
    char *to_send = malloc(len + 2);
    strcpy(to_send, string);
    strcat(to_send, "\n");

    send(socket, to_send, len + 1, 0);

    free(to_send);
    cJSON_free(string);
}

void send_error(int socket, const char *msg) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "ERROR");
    if (msg) {
        cJSON_AddStringToObject(resp, "msg", msg);
    }
    send_json(socket, resp);
    cJSON_Delete(resp);
}

void send_ok(int socket) {
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddStringToObject(resp, "status", "OK");
    send_json(socket, resp);
    cJSON_Delete(resp);
}

