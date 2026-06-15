#include "chat_server.hpp"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <csignal>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

static constexpr int MAX_EVENTS   = 64;
static constexpr int BACKLOG      = 128;
static constexpr size_t READ_BUF  = 4096;
static constexpr size_t HISTORIAL_ENVIAR = 20;

std::atomic<bool> ChatServer::corriendo_{true};

// ── Constructor / destructor ─────────────────────────────────────────────────

ChatServer::ChatServer(int puerto) : puerto_(puerto) {
    std::signal(SIGINT,  ChatServer::handle_signal);
    std::signal(SIGTERM, ChatServer::handle_signal);
    fs::create_directories("logs");
    registrar_sala_default();
    setup_socket();
    setup_epoll();
}

ChatServer::~ChatServer() {
    for (auto& [fd, _] : clientes_) close(fd);
    if (epoll_fd_  >= 0) close(epoll_fd_);
    if (server_fd_ >= 0) close(server_fd_);
}

void ChatServer::handle_signal(int) { corriendo_ = false; }

// ── Setup ────────────────────────────────────────────────────────────────────

void ChatServer::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl F_SETFL O_NONBLOCK");
}

void ChatServer::setup_socket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) throw std::runtime_error("socket()");

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd_);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(static_cast<uint16_t>(puerto_));

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind()");

    if (listen(server_fd_, BACKLOG) < 0)
        throw std::runtime_error("listen()");

    std::cout << "[Servidor] Escuchando en el puerto " << puerto_ << "\n";
}

void ChatServer::setup_epoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1()");

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) < 0)
        throw std::runtime_error("epoll_ctl(server_fd)");
}

void ChatServer::registrar_sala_default() {
    salas_.emplace("general", Sala{"general"});
    cargar_historial(salas_.at("general"));
}

// ── Loop principal ───────────────────────────────────────────────────────────

void ChatServer::run() {
    epoll_event eventos[MAX_EVENTS];

    while (corriendo_) {
        int n = epoll_wait(epoll_fd_, eventos, MAX_EVENTS, 500);
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        for (int i = 0; i < n; ++i) {
            int fd = eventos[i].data.fd;
            if (fd == server_fd_) {
                aceptar_conexion();
            } else {
                if (eventos[i].events & (EPOLLERR | EPOLLHUP)) {
                    desconectar_cliente(fd);
                } else if (eventos[i].events & EPOLLIN) {
                    leer_de_cliente(fd);
                } else if (eventos[i].events & EPOLLOUT) {
                    escribir_a_cliente(fd);
                }
            }
        }
    }
    std::cout << "\n[Servidor] Cerrando...\n";
}

// ── Aceptar conexión ─────────────────────────────────────────────────────────

void ChatServer::aceptar_conexion() {
    while (true) {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        int fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            std::cerr << "[Error] accept: " << strerror(errno) << "\n";
            break;
        }
        set_nonblocking(fd);

        epoll_event ev{};
        ev.events  = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);

        clientes_.emplace(fd, Cliente{fd, "", "", "", {}});

        // Enviar bienvenida con lista de salas
        enviar(fd, proto::bienvenida("Bienvenido al Chat Server. Regístrate con {\"tipo\":\"unirse\",\"sala\":\"general\",\"usuario\":\"TuNombre\"}"));
        enviar(fd, proto::lista_salas(nombres_salas()));

        std::cout << "[+] Nueva conexión fd=" << fd
                  << " desde " << inet_ntoa(addr.sin_addr) << "\n";
    }
}

// ── Lectura ──────────────────────────────────────────────────────────────────

void ChatServer::leer_de_cliente(int fd) {
    char buf[READ_BUF];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
                desconectar_cliente(fd);
            break;
        }
        auto it = clientes_.find(fd);
        if (it == clientes_.end()) break;

        it->second.buffer_entrada.append(buf, static_cast<size_t>(n));

        // Procesar todos los mensajes completos (delimitados por \n)
        auto& buffer = it->second.buffer_entrada;
        size_t pos;
        while ((pos = buffer.find('\n')) != std::string::npos) {
            std::string linea = buffer.substr(0, pos);
            buffer.erase(0, pos + 1);
            if (linea.empty()) continue;
            try {
                json msg = json::parse(linea);
                procesar_mensaje(fd, msg);
            } catch (const json::parse_error& e) {
                enviar(fd, proto::error_msg(std::string("JSON inválido: ") + e.what()));
            }
        }
    }
}

// ── Escritura ────────────────────────────────────────────────────────────────

void ChatServer::escribir_a_cliente(int fd) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    auto& cola = it->second.cola_salida;
    while (!cola.empty()) {
        const std::string& msg = cola.front();
        ssize_t enviado = send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        if (enviado < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            desconectar_cliente(fd);
            return;
        }
        cola.pop_front();
    }
    if (cola.empty()) epoll_mod_out(fd, false);
}

void ChatServer::epoll_mod_out(int fd, bool activar) {
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events  = EPOLLIN | EPOLLET | (activar ? static_cast<uint32_t>(EPOLLOUT) : 0u);
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

// ── Desconexión ──────────────────────────────────────────────────────────────

void ChatServer::desconectar_cliente(int fd) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    const std::string& nombre    = it->second.nombre;
    const std::string& sala_name = it->second.sala_actual;

    if (!sala_name.empty() && salas_.count(sala_name)) {
        auto& sala = salas_.at(sala_name);
        sala.quitar_miembro(fd);
        if (!nombre.empty()) {
            broadcast_sala(sala_name,
                proto::sistema(sala_name, nombre + " salió de la sala"), fd);
        }
    }
    std::cout << "[-] Desconectado fd=" << fd;
    if (!nombre.empty()) std::cout << " (" << nombre << ")";
    std::cout << "\n";

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clientes_.erase(it);
}

// ── Procesamiento de mensajes ────────────────────────────────────────────────

void ChatServer::procesar_mensaje(int fd, const json& msg) {
    if (!msg.contains("tipo")) {
        enviar(fd, proto::error_msg("Falta campo 'tipo'"));
        return;
    }
    std::string tipo = msg["tipo"].get<std::string>();

    if      (tipo == T_UNIRSE)       cmd_unirse(fd, msg);
    else if (tipo == T_CREAR_SALA)   cmd_crear_sala(fd, msg);
    else if (tipo == T_MENSAJE)      cmd_mensaje(fd, msg);
    else if (tipo == T_IMAGEN)       cmd_imagen(fd, msg);
    else if (tipo == T_LISTAR_SALAS) cmd_listar_salas(fd);
    else if (tipo == T_LISTAR_USERS) cmd_listar_usuarios(fd, msg);
    else if (tipo == T_SALIR)        cmd_salir(fd);
    else enviar(fd, proto::error_msg("Tipo desconocido: " + tipo));
}

// ── Comandos ─────────────────────────────────────────────────────────────────

void ChatServer::cmd_unirse(int fd, const json& msg) {
    if (!msg.contains("usuario") || !msg.contains("sala")) {
        enviar(fd, proto::error_msg("Faltan campos: usuario, sala"));
        return;
    }
    std::string nombre = msg["usuario"].get<std::string>();
    std::string sala_n = msg["sala"].get<std::string>();

    if (nombre.empty() || sala_n.empty()) {
        enviar(fd, proto::error_msg("usuario y sala no pueden estar vacíos"));
        return;
    }

    // Verificar nombre duplicado
    for (auto& [ofd, c] : clientes_) {
        if (ofd != fd && c.nombre == nombre) {
            enviar(fd, proto::error_msg("El nombre '" + nombre + "' ya está en uso"));
            return;
        }
    }

    // Salir de sala anterior si existía
    auto& cliente = clientes_.at(fd);
    if (!cliente.sala_actual.empty() && salas_.count(cliente.sala_actual)) {
        salas_.at(cliente.sala_actual).quitar_miembro(fd);
        broadcast_sala(cliente.sala_actual,
            proto::sistema(cliente.sala_actual, nombre + " cambió de sala"), fd);
    }

    if (!salas_.count(sala_n)) {
        salas_.emplace(sala_n, Sala{sala_n});
        cargar_historial(salas_.at(sala_n));
    }

    cliente.nombre      = nombre;
    cliente.sala_actual = sala_n;
    salas_.at(sala_n).agregar_miembro(fd);

    // Historial
    auto hist = salas_.at(sala_n).ultimos_mensajes(HISTORIAL_ENVIAR);
    if (!hist.empty())
        enviar(fd, proto::historial(sala_n, hist));

    // Confirmación y estado de la sala
    enviar(fd, proto::lista_salas(nombres_salas()));
    enviar(fd, proto::lista_usuarios(sala_n, usuarios_en_sala(sala_n)));

    // Notificar al resto
    broadcast_sala(sala_n,
        proto::sistema(sala_n, nombre + " se unió a la sala"), fd);

    std::cout << "[*] " << nombre << " se unió a #" << sala_n << "\n";
}

void ChatServer::cmd_crear_sala(int fd, const json& msg) {
    if (!msg.contains("nombre") || !msg.contains("usuario")) {
        enviar(fd, proto::error_msg("Faltan campos: nombre, usuario"));
        return;
    }
    std::string sala_n = msg["nombre"].get<std::string>();
    std::string nombre = msg["usuario"].get<std::string>();

    if (salas_.count(sala_n)) {
        enviar(fd, proto::error_msg("La sala '" + sala_n + "' ya existe"));
        return;
    }

    salas_.emplace(sala_n, Sala{sala_n});

    // Notificar a todos los clientes de la nueva sala
    json notif = proto::lista_salas(nombres_salas());
    for (auto& [ofd, _] : clientes_) enviar(ofd, notif);

    std::cout << "[*] " << nombre << " creó la sala #" << sala_n << "\n";
}

void ChatServer::cmd_mensaje(int fd, const json& msg) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    if (!it->second.registrado()) {
        enviar(fd, proto::error_msg("Debes unirte a una sala primero"));
        return;
    }
    if (!msg.contains("contenido")) {
        enviar(fd, proto::error_msg("Falta campo 'contenido'"));
        return;
    }

    const std::string& sala_n    = it->second.sala_actual;
    const std::string& nombre    = it->second.nombre;
    std::string        contenido = msg["contenido"].get<std::string>();

    json bcast = proto::broadcast(sala_n, nombre, contenido);

    // Guardar en historial (memoria + archivo)
    salas_.at(sala_n).guardar_en_historial(bcast);
    guardar_mensaje_log(sala_n, bcast);

    // Broadcast incluyendo al remitente (eco de confirmación)
    broadcast_sala(sala_n, bcast);
}

void ChatServer::cmd_imagen(int fd, const json& msg) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    if (!it->second.registrado()) {
        enviar(fd, proto::error_msg("Debes unirte a una sala primero"));
        return;
    }
    if (!msg.contains("datos") || !msg.contains("nombre_archivo")) {
        enviar(fd, proto::error_msg("Faltan campos: nombre_archivo, datos"));
        return;
    }

    const std::string& sala_n = it->second.sala_actual;
    const std::string& nombre = it->second.nombre;
    std::string archivo = msg["nombre_archivo"].get<std::string>();
    std::string datos   = msg["datos"].get<std::string>();

    // Limite de seguridad: ~3MB de base64 (~2.2MB de imagen real)
    if (datos.size() > 3u * 1024 * 1024) {
        enviar(fd, proto::error_msg("Imagen demasiado grande (max ~2MB)"));
        return;
    }

    json img = proto::imagen(sala_n, nombre, archivo, datos);

    // Reenviar la imagen completa a todos los miembros de la sala (incluido el remitente)
    broadcast_sala(sala_n, img);

    // En el historial guardamos solo una nota ligera (no el payload), para no inflar el log
    json nota = proto::sistema(sala_n, nombre + " compartio la imagen: " + archivo);
    salas_.at(sala_n).guardar_en_historial(nota);
    guardar_mensaje_log(sala_n, nota);

    std::cout << "[img] " << nombre << " compartio " << archivo
              << " (" << datos.size() << " bytes b64) en #" << sala_n << "\n";
}

void ChatServer::cmd_listar_salas(int fd) {
    enviar(fd, proto::lista_salas(nombres_salas()));
}

void ChatServer::cmd_listar_usuarios(int fd, const json& msg) {
    std::string sala_n = msg.value("sala", "");
    if (sala_n.empty()) {
        auto it = clientes_.find(fd);
        if (it != clientes_.end()) sala_n = it->second.sala_actual;
    }
    if (!salas_.count(sala_n)) {
        enviar(fd, proto::error_msg("Sala no encontrada: " + sala_n));
        return;
    }
    enviar(fd, proto::lista_usuarios(sala_n, usuarios_en_sala(sala_n)));
}

void ChatServer::cmd_salir(int fd) {
    desconectar_cliente(fd);
}

// ── Envío ────────────────────────────────────────────────────────────────────

void ChatServer::enviar(int fd, const json& msg) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    std::string serializado = proto::serialize(msg);
    auto& cola = it->second.cola_salida;

    if (cola.empty()) {
        // Intentar envío directo
        ssize_t n = send(fd, serializado.c_str(), serializado.size(), MSG_NOSIGNAL);
        if (n == static_cast<ssize_t>(serializado.size())) return;
        if (n > 0) serializado = serializado.substr(static_cast<size_t>(n));
        if (errno == EAGAIN || errno == EWOULDBLOCK || n > 0) {
            cola.push_back(std::move(serializado));
            epoll_mod_out(fd, true);
            return;
        }
        desconectar_cliente(fd);
        return;
    }
    cola.push_back(std::move(serializado));
    epoll_mod_out(fd, true);
}

void ChatServer::broadcast_sala(const std::string& sala, const json& msg, int excluir_fd) {
    if (!salas_.count(sala)) return;
    for (int fd : salas_.at(sala).miembros_fd) {
        if (fd != excluir_fd) enviar(fd, msg);
    }
}

// ── Historial ────────────────────────────────────────────────────────────────

std::string ChatServer::ruta_log(const std::string& sala) const {
    return "logs/" + sala + ".log";
}

void ChatServer::cargar_historial(Sala& sala) {
    std::ifstream f(ruta_log(sala.nombre));
    if (!f.is_open()) return;
    std::string linea;
    while (std::getline(f, linea)) {
        if (linea.empty()) continue;
        try {
            sala.guardar_en_historial(json::parse(linea));
        } catch (...) {}
    }
}

void ChatServer::guardar_mensaje_log(const std::string& sala, const json& msg) {
    std::ofstream f(ruta_log(sala), std::ios::app);
    if (f.is_open()) f << msg.dump() << "\n";
}

// ── Utilidades ───────────────────────────────────────────────────────────────

std::vector<std::string> ChatServer::nombres_salas() const {
    std::vector<std::string> v;
    v.reserve(salas_.size());
    for (auto& [k, _] : salas_) v.push_back(k);
    return v;
}

std::vector<std::string> ChatServer::usuarios_en_sala(const std::string& sala) const {
    std::vector<std::string> v;
    if (!salas_.count(sala)) return v;
    for (int fd : salas_.at(sala).miembros_fd) {
        auto it = clientes_.find(fd);
        if (it != clientes_.end() && !it->second.nombre.empty())
            v.push_back(it->second.nombre);
    }
    return v;
}
