// ============================================================================
//  chat_client.cpp
//  ----------------------------------------------------------------------------
//  Implementacion de la capa de red del cliente (ver chat_client.hpp).
// ============================================================================
#include "chat_client.hpp"
#include "base64.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>          // getaddrinfo (resolver host)
#include <unistd.h>         // close

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <cstdlib>          // getenv (para expandir '~')

// Constructor: solo guarda los datos; la conexion se hace en conectar().
ChatClient::ChatClient(std::string host, int puerto, std::string usuario)
    : host_(std::move(host)), puerto_(puerto), usuario_(std::move(usuario)) {}

ChatClient::~ChatClient() {
    detener();
}

// Resuelve el host (acepta IP o nombre) y abre una conexion TCP bloqueante.
// Devuelve true si connect() tuvo exito.
bool ChatClient::conectar() {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM;   // TCP

    std::string puerto_str = std::to_string(puerto_);
    if (getaddrinfo(host_.c_str(), puerto_str.c_str(), &hints, &res) != 0)
        return false;

    socket_fd_ = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socket_fd_ < 0) { freeaddrinfo(res); return false; }

    if (connect(socket_fd_, res->ai_addr, res->ai_addrlen) < 0) {
        close(socket_fd_);
        socket_fd_ = -1;
        freeaddrinfo(res);
        return false;
    }
    freeaddrinfo(res);
    return true;
}

// Lanza el hilo que escuchara el socket. 'cb' es el callback que recibira cada
// mensaje del servidor (la TUI pasa aqui su on_mensaje).
void ChatClient::iniciar_recepcion(MsgCallback cb) {
    callback_ = std::move(cb);
    corriendo_ = true;
    hilo_rx_ = std::thread([this] { loop_recepcion(); });
}

// Para el hilo y cierra el socket de forma segura (idempotente).
// shutdown() desbloquea el recv() del hilo para que pueda terminar.
void ChatClient::detener() {
    corriendo_ = false;
    if (socket_fd_ >= 0) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }
    if (hilo_rx_.joinable()) hilo_rx_.join();
}

// Serializa el JSON (con su '\n') y lo manda por el socket. MSG_NOSIGNAL evita
// recibir SIGPIPE si el servidor cerro la conexion.
void ChatClient::enviar_json(const json& msg) {
    if (socket_fd_ < 0) return;
    std::string s = proto::serialize(msg);
    send(socket_fd_, s.c_str(), s.size(), MSG_NOSIGNAL);
}

// ── Comandos: cada uno arma el JSON correcto y lo manda ──────────────────────

void ChatClient::enviar_unirse(const std::string& sala) {
    enviar_json({ {"tipo", T_UNIRSE}, {"sala", sala}, {"usuario", usuario_} });
}

void ChatClient::enviar_mensaje(const std::string& contenido) {
    enviar_json({ {"tipo", T_MENSAJE}, {"sala", sala_actual_},
                  {"usuario", usuario_}, {"contenido", contenido} });
}

// Lee un archivo de imagen y lo envia codificado en base64.
// Devuelve "" si todo fue bien, o un texto de error para mostrar en la TUI.
std::string ChatClient::enviar_imagen(const std::string& ruta_in) {
    namespace fs = std::filesystem;

    // La TUI no pasa por la shell, asi que '~' llega literal: lo expandimos a HOME.
    std::string ruta = ruta_in;
    if (!ruta.empty() && ruta[0] == '~' &&
        (ruta.size() == 1 || ruta[1] == '/')) {
        const char* home = std::getenv("HOME");
        if (home) ruta = std::string(home) + ruta.substr(1);
    }

    // Validaciones: existe, se puede medir, no excede 2MB.
    std::error_code ec;
    if (!fs::exists(ruta, ec))
        return "El archivo no existe: " + ruta;

    auto tam = fs::file_size(ruta, ec);
    if (ec) return "No se pudo leer el archivo";
    if (tam > 2u * 1024 * 1024)
        return "Imagen demasiado grande (max 2MB)";

    // Leer el archivo completo en binario.
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return "No se pudo abrir el archivo";

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    if (bytes.empty()) return "El archivo esta vacio";

    // Solo el nombre (sin la ruta) viaja al servidor; el contenido va en base64.
    std::string nombre = fs::path(ruta).filename().string();
    std::string datos  = b64::encode(bytes);

    enviar_json(proto::imagen(sala_actual_, usuario_, nombre, datos));
    return "";
}

void ChatClient::enviar_crear_sala(const std::string& sala) {
    enviar_json({ {"tipo", T_CREAR_SALA}, {"nombre", sala}, {"usuario", usuario_} });
}

void ChatClient::enviar_listar_salas() {
    enviar_json({ {"tipo", T_LISTAR_SALAS} });
}

// Si 'sala' va vacia, el servidor usa la sala actual del cliente.
void ChatClient::enviar_listar_usuarios(const std::string& sala) {
    json msg = { {"tipo", T_LISTAR_USERS} };
    if (!sala.empty()) msg["sala"] = sala;
    enviar_json(msg);
}

void ChatClient::enviar_salir() {
    enviar_json({ {"tipo", T_SALIR} });
}

// ── Hilo de recepcion ────────────────────────────────────────────────────────

// Bucle del hilo: lee del socket, rearma los mensajes por '\n' (igual que el
// servidor) y entrega cada JSON al callback. Al perder la conexion, manda un
// mensaje sintetico {"tipo":"desconectado"} para que la TUI lo sepa y cierre.
void ChatClient::loop_recepcion() {
    char buf[4096];
    while (corriendo_) {
        ssize_t n = recv(socket_fd_, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { corriendo_ = false; break; }  // 0 = cerro, <0 = error

        buffer_entrada_.append(buf, static_cast<size_t>(n));

        size_t pos;
        while ((pos = buffer_entrada_.find('\n')) != std::string::npos) {
            std::string linea = buffer_entrada_.substr(0, pos);
            buffer_entrada_.erase(0, pos + 1);
            if (linea.empty()) continue;
            try {
                json msg = json::parse(linea);
                if (callback_) callback_(msg);
            } catch (...) {}   // ignorar lineas mal formadas
        }
    }
    // Avisar a la TUI que ya no hay conexion.
    if (callback_)
        callback_({ {"tipo", "desconectado"} });
}
