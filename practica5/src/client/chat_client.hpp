#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include "../common/protocolo.hpp"

// Callback que la TUI registra para recibir mensajes del servidor
using MsgCallback = std::function<void(const json&)>;

class ChatClient {
public:
    ChatClient(std::string host, int puerto, std::string usuario);
    ~ChatClient();

    bool conectar();
    void iniciar_recepcion(MsgCallback cb);
    void detener();

    void enviar_unirse(const std::string& sala);
    void enviar_mensaje(const std::string& contenido);
    // Devuelve "" si tuvo exito, o un mensaje de error en caso contrario.
    std::string enviar_imagen(const std::string& ruta);
    void enviar_crear_sala(const std::string& sala);
    void enviar_listar_salas();
    void enviar_listar_usuarios(const std::string& sala = "");
    void enviar_salir();

    bool conectado() const { return socket_fd_ >= 0; }
    const std::string& usuario() const { return usuario_; }
    const std::string& sala_actual() const { return sala_actual_; }
    void set_sala_actual(const std::string& s) { sala_actual_ = s; }

private:
    std::string host_;
    int         puerto_;
    std::string usuario_;
    std::string sala_actual_;
    int         socket_fd_ = -1;

    std::thread     hilo_rx_;
    std::atomic<bool> corriendo_{false};
    MsgCallback     callback_;

    std::string buffer_entrada_;

    void enviar_json(const json& msg);
    void loop_recepcion();
};
