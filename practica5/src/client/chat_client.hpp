#pragma once
// ============================================================================
//  chat_client.hpp
//  ----------------------------------------------------------------------------
//  Logica de RED del cliente: conectar al servidor, enviar comandos y recibir
//  mensajes. A diferencia del servidor, el cliente SI usa un hilo aparte:
//   - El hilo principal corre la TUI (ncurses) y lee el teclado.
//   - Un hilo de recepcion (hilo_rx_) lee del socket y, por cada mensaje JSON
//     completo, llama a un callback que la TUI registra (on_mensaje).
//  El acceso al estado compartido lo protege la TUI con su propio mutex.
// ============================================================================
#include <string>
#include <thread>
#include <atomic>
#include <functional>
#include "../common/protocolo.hpp"

// Tipo del callback con el que la TUI recibe cada mensaje del servidor.
using MsgCallback = std::function<void(const json&)>;

class ChatClient {
public:
    // Guarda host/puerto/usuario, pero NO conecta todavia (eso es conectar()).
    ChatClient(std::string host, int puerto, std::string usuario);
    ~ChatClient();   // asegura detener() (cierra socket y une el hilo)

    // Abre la conexion TCP al servidor. Devuelve true si lo logra.
    bool conectar();
    // Arranca el hilo de recepcion y registra el callback de mensajes.
    void iniciar_recepcion(MsgCallback cb);
    // Detiene el hilo de recepcion y cierra el socket de forma segura.
    void detener();

    // ── Comandos hacia el servidor (cada uno arma y envia un JSON) ───────────
    void enviar_unirse(const std::string& sala);
    void enviar_mensaje(const std::string& contenido);
    // Lee un archivo de imagen, lo codifica en base64 y lo envia.
    // Devuelve "" si tuvo exito, o un mensaje de error en caso contrario.
    std::string enviar_imagen(const std::string& ruta);
    void enviar_crear_sala(const std::string& sala);
    void enviar_listar_salas();
    void enviar_listar_usuarios(const std::string& sala = "");
    void enviar_salir();

    // ── Accesores de estado ──────────────────────────────────────────────────
    bool conectado() const { return socket_fd_ >= 0; }
    const std::string& usuario() const { return usuario_; }
    const std::string& sala_actual() const { return sala_actual_; }
    void set_sala_actual(const std::string& s) { sala_actual_ = s; }

private:
    std::string host_;
    int         puerto_;
    std::string usuario_;
    std::string sala_actual_;
    int         socket_fd_ = -1;        // -1 = sin conexion

    std::thread       hilo_rx_;         // hilo que escucha el socket
    std::atomic<bool> corriendo_{false}; // bandera de vida del hilo de recepcion
    MsgCallback       callback_;        // a quien entregar cada mensaje recibido

    std::string buffer_entrada_;        // acumulador para el framing por '\n'

    void enviar_json(const json& msg);  // serializa y manda por el socket
    void loop_recepcion();              // cuerpo del hilo de recepcion
};
