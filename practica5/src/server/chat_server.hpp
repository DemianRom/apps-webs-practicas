#pragma once
#include <string>
#include <unordered_map>
#include <atomic>
#include "cliente.hpp"
#include "sala.hpp"
#include "../common/protocolo.hpp"

class ChatServer {
public:
    explicit ChatServer(int puerto);
    ~ChatServer();

    void run();
    static void handle_signal(int sig);

private:
    int puerto_;
    int server_fd_ = -1;
    int epoll_fd_  = -1;

    std::unordered_map<int, Cliente> clientes_;
    std::unordered_map<std::string, Sala> salas_;

    static std::atomic<bool> corriendo_;

    // ── Setup ────────────────────────────────────────────────────────────────
    void setup_socket();
    void setup_epoll();
    void set_nonblocking(int fd);
    void registrar_sala_default();

    // ── Loop principal ───────────────────────────────────────────────────────
    void aceptar_conexion();
    void leer_de_cliente(int fd);
    void escribir_a_cliente(int fd);
    void desconectar_cliente(int fd);

    // ── Protocolo ────────────────────────────────────────────────────────────
    void procesar_mensaje(int fd, const json& msg);
    void cmd_unirse(int fd, const json& msg);
    void cmd_crear_sala(int fd, const json& msg);
    void cmd_mensaje(int fd, const json& msg);
    void cmd_imagen(int fd, const json& msg);
    void cmd_listar_salas(int fd);
    void cmd_listar_usuarios(int fd, const json& msg);
    void cmd_salir(int fd);

    // ── Envío ────────────────────────────────────────────────────────────────
    void enviar(int fd, const json& msg);
    void broadcast_sala(const std::string& sala, const json& msg, int excluir_fd = -1);

    // ── Historial ────────────────────────────────────────────────────────────
    void cargar_historial(Sala& sala);
    void guardar_mensaje_log(const std::string& sala, const json& msg);
    std::string ruta_log(const std::string& sala) const;

    // ── Utilidades ───────────────────────────────────────────────────────────
    std::vector<std::string> nombres_salas() const;
    std::vector<std::string> usuarios_en_sala(const std::string& sala) const;
    void epoll_mod_out(int fd, bool activar);
};
