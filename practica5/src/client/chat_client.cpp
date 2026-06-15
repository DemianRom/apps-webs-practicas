#include "chat_client.hpp"
#include "base64.hpp"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <iostream>
#include <stdexcept>
#include <cstring>
#include <fstream>
#include <filesystem>

ChatClient::ChatClient(std::string host, int puerto, std::string usuario)
    : host_(std::move(host)), puerto_(puerto), usuario_(std::move(usuario)) {}

ChatClient::~ChatClient() {
    detener();
}

bool ChatClient::conectar() {
    addrinfo hints{}, *res = nullptr;
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

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

void ChatClient::iniciar_recepcion(MsgCallback cb) {
    callback_ = std::move(cb);
    corriendo_ = true;
    hilo_rx_ = std::thread([this] { loop_recepcion(); });
}

void ChatClient::detener() {
    corriendo_ = false;
    if (socket_fd_ >= 0) {
        shutdown(socket_fd_, SHUT_RDWR);
        close(socket_fd_);
        socket_fd_ = -1;
    }
    if (hilo_rx_.joinable()) hilo_rx_.join();
}

void ChatClient::enviar_json(const json& msg) {
    if (socket_fd_ < 0) return;
    std::string s = proto::serialize(msg);
    send(socket_fd_, s.c_str(), s.size(), MSG_NOSIGNAL);
}

void ChatClient::enviar_unirse(const std::string& sala) {
    enviar_json({ {"tipo", T_UNIRSE}, {"sala", sala}, {"usuario", usuario_} });
}

void ChatClient::enviar_mensaje(const std::string& contenido) {
    enviar_json({ {"tipo", T_MENSAJE}, {"sala", sala_actual_},
                  {"usuario", usuario_}, {"contenido", contenido} });
}

std::string ChatClient::enviar_imagen(const std::string& ruta) {
    namespace fs = std::filesystem;
    std::error_code ec;
    if (!fs::exists(ruta, ec))
        return "El archivo no existe: " + ruta;

    auto tam = fs::file_size(ruta, ec);
    if (ec) return "No se pudo leer el archivo";
    if (tam > 2u * 1024 * 1024)
        return "Imagen demasiado grande (max 2MB)";

    std::ifstream f(ruta, std::ios::binary);
    if (!f) return "No se pudo abrir el archivo";

    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(f)),
                               std::istreambuf_iterator<char>());
    if (bytes.empty()) return "El archivo esta vacio";

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

void ChatClient::enviar_listar_usuarios(const std::string& sala) {
    json msg = { {"tipo", T_LISTAR_USERS} };
    if (!sala.empty()) msg["sala"] = sala;
    enviar_json(msg);
}

void ChatClient::enviar_salir() {
    enviar_json({ {"tipo", T_SALIR} });
}

void ChatClient::loop_recepcion() {
    char buf[4096];
    while (corriendo_) {
        ssize_t n = recv(socket_fd_, buf, sizeof(buf) - 1, 0);
        if (n <= 0) { corriendo_ = false; break; }

        buffer_entrada_.append(buf, static_cast<size_t>(n));

        size_t pos;
        while ((pos = buffer_entrada_.find('\n')) != std::string::npos) {
            std::string linea = buffer_entrada_.substr(0, pos);
            buffer_entrada_.erase(0, pos + 1);
            if (linea.empty()) continue;
            try {
                json msg = json::parse(linea);
                if (callback_) callback_(msg);
            } catch (...) {}
        }
    }
    // Notificar a la TUI que se perdió la conexión
    if (callback_)
        callback_({ {"tipo", "desconectado"} });
}
