/*
 * Practica 5 - Servidor de chat no bloqueante
 * Materia: Aplicaciones y Comunicaciones en Red (6CM1)
 * ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
 * Periodo: 26/2
 *
 * Integrantes:
 * - Romero Bautista Demian
 * - Ferreira Rodriguez Said
 *
 * Responsabilidad del archivo:
 * Declara la clase ChatServer y sus estructuras para administrar el ciclo no bloqueante del servidor.
 */

#pragma once
// ============================================================================
//  chat_server.hpp
//  ----------------------------------------------------------------------------
//  Declaracion de la clase ChatServer: el corazon del servidor no bloqueante.
//  Mantiene el socket de escucha, el epoll, el mapa de clientes conectados y el
//  mapa de salas. Un unico hilo atiende todo mediante epoll (sin threads por
//  cliente). La implementacion esta en chat_server.cpp.
// ============================================================================
#include <string>
#include <unordered_map>
#include <atomic>
#include "cliente.hpp"
#include "sala.hpp"
#include "../common/protocolo.hpp"

class ChatServer {
public:
    // Crea el servidor: prepara senales, carpeta de logs, sala "general",
    // el socket de escucha y el epoll. Lanza std::runtime_error si algo falla.
    explicit ChatServer(int puerto);

    // Cierra todos los sockets abiertos (clientes, epoll y escucha).
    ~ChatServer();

    // Bucle principal: epoll_wait + atencion de eventos hasta recibir SIGINT.
    void run();

    // Manejador de SIGINT/SIGTERM: pone corriendo_ = false para salir del bucle.
    static void handle_signal(int sig);

private:
    int puerto_;
    int server_fd_ = -1;   // socket TCP de escucha
    int epoll_fd_  = -1;   // instancia de epoll

    std::unordered_map<int, Cliente> clientes_;        // fd        -> datos del cliente
    std::unordered_map<std::string, Sala> salas_;      // nombre    -> sala

    // Bandera global de "seguir corriendo". Es static + atomic porque la toca
    // el manejador de senales (handle_signal), que no recibe puntero a 'this'.
    static std::atomic<bool> corriendo_;

    // ── Setup (preparacion inicial) ──────────────────────────────────────────
    void setup_socket();           // crea/bind/listen del socket de escucha
    void setup_epoll();            // crea epoll y registra el socket de escucha
    void set_nonblocking(int fd);  // marca un fd como O_NONBLOCK
    void registrar_sala_default(); // crea la sala "general" y carga su log

    // ── Bucle principal / E/S de sockets ─────────────────────────────────────
    void aceptar_conexion();           // accept() de conexiones nuevas
    void leer_de_cliente(int fd);      // recv() + extraer mensajes por '\n'
    void escribir_a_cliente(int fd);   // vaciar la cola de salida pendiente
    void desconectar_cliente(int fd);  // cerrar y limpiar todo lo del cliente

    // ── Procesamiento del protocolo (despacho por "tipo") ────────────────────
    void procesar_mensaje(int fd, const json& msg);   // mira "tipo" y delega
    void cmd_unirse(int fd, const json& msg);
    void cmd_crear_sala(int fd, const json& msg);
    void cmd_mensaje(int fd, const json& msg);
    void cmd_imagen(int fd, const json& msg);
    void cmd_listar_salas(int fd);
    void cmd_listar_usuarios(int fd, const json& msg);
    void cmd_salir(int fd);

    // ── Envio de datos ───────────────────────────────────────────────────────
    void enviar(int fd, const json& msg);             // a un cliente (con cola)
    // a todos los de una sala; excluir_fd permite no reenviar al remitente.
    void broadcast_sala(const std::string& sala, const json& msg, int excluir_fd = -1);

    // ── Historial persistente (archivos en logs/) ────────────────────────────
    void cargar_historial(Sala& sala);                            // log -> memoria
    void guardar_mensaje_log(const std::string& sala, const json& msg); // append a log
    std::string ruta_log(const std::string& sala) const;          // "logs/<sala>.log"

    // ── Utilidades ───────────────────────────────────────────────────────────
    std::vector<std::string> nombres_salas() const;                 // todas las salas
    std::vector<std::string> usuarios_en_sala(const std::string& sala) const;
    // Activa/desactiva el interes en EPOLLOUT para un fd (cuando hay cola pendiente).
    void epoll_mod_out(int fd, bool activar);
};
