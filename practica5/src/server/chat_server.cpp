// ============================================================================
//  chat_server.cpp
//  ----------------------------------------------------------------------------
//  Implementacion del servidor de chat NO BLOQUEANTE con epoll.
//
//  Modelo de E/S (resumen para repasar):
//   1. El socket de escucha y los de clientes estan en O_NONBLOCK: ninguna
//      llamada (accept/recv/send) se queda esperando; si no hay datos devuelven
//      EAGAIN/EWOULDBLOCK y seguimos.
//   2. epoll nos dice EN QUE fd hay algo que hacer, sin recorrer todos.
//      Usamos modo edge-triggered (EPOLLET): hay que leer/escribir en bucle
//      hasta agotar (EAGAIN), porque epoll solo avisa en los "flancos".
//   3. Un solo hilo atiende a todos los clientes. No hay threads por conexion.
// ============================================================================
#include "chat_server.hpp"

#include <sys/epoll.h>      // epoll_create1, epoll_ctl, epoll_wait
#include <sys/socket.h>     // socket, bind, listen, accept, recv, send
#include <netinet/in.h>     // sockaddr_in
#include <arpa/inet.h>      // inet_ntoa
#include <fcntl.h>          // fcntl (O_NONBLOCK)
#include <unistd.h>         // close
#include <csignal>          // signal, SIGINT

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

// Parametros de ajuste del servidor.
static constexpr int MAX_EVENTS   = 64;    // eventos que epoll_wait reporta por vuelta
static constexpr int BACKLOG      = 128;   // cola de conexiones pendientes en listen()
static constexpr size_t READ_BUF  = 4096;  // tamano del buffer de lectura por recv()
static constexpr size_t HISTORIAL_ENVIAR = 20; // cuantos mensajes mandar al entrar

// Definicion del miembro static. Empieza en true; una senal lo pone en false.
std::atomic<bool> ChatServer::corriendo_{true};

// ── Constructor / destructor ─────────────────────────────────────────────────

// Prepara TODO lo necesario para arrancar. Si algo critico falla, lanza una
// excepcion que main_server.cpp captura y reporta.
ChatServer::ChatServer(int puerto) : puerto_(puerto) {
    // Capturar Ctrl+C y kill para poder cerrar de forma ordenada.
    std::signal(SIGINT,  ChatServer::handle_signal);
    std::signal(SIGTERM, ChatServer::handle_signal);
    fs::create_directories("logs");   // carpeta de historiales
    registrar_sala_default();         // crear sala "general" (+ cargar su log)
    setup_socket();                   // socket de escucha listo
    setup_epoll();                    // epoll listo y vigilando el socket
}

// Cierra ordenadamente: primero los sockets de clientes, luego epoll y escucha.
ChatServer::~ChatServer() {
    for (auto& [fd, _] : clientes_) close(fd);
    if (epoll_fd_  >= 0) close(epoll_fd_);
    if (server_fd_ >= 0) close(server_fd_);
}

// Manejador de senal. Debe ser minimo: solo cambia la bandera atomica para que
// el bucle principal termine en su proxima vuelta.
void ChatServer::handle_signal(int) { corriendo_ = false; }

// ── Setup ────────────────────────────────────────────────────────────────────

// Pone un descriptor en modo no bloqueante (O_NONBLOCK) preservando sus flags.
void ChatServer::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) throw std::runtime_error("fcntl F_GETFL");
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        throw std::runtime_error("fcntl F_SETFL O_NONBLOCK");
}

// Crea el socket TCP de escucha: socket -> SO_REUSEADDR -> non-blocking ->
// bind a todas las interfaces en 'puerto_' -> listen.
void ChatServer::setup_socket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) throw std::runtime_error("socket()");

    // SO_REUSEADDR: permite reusar el puerto enseguida tras reiniciar el server
    // (evita el clasico "Address already in use").
    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    set_nonblocking(server_fd_);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;                       // acepta de cualquier IP
    addr.sin_port        = htons(static_cast<uint16_t>(puerto_)); // a orden de red

    if (bind(server_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        throw std::runtime_error("bind()");

    if (listen(server_fd_, BACKLOG) < 0)
        throw std::runtime_error("listen()");

    std::cout << "[Servidor] Escuchando en el puerto " << puerto_ << "\n";
}

// Crea la instancia de epoll y registra el socket de escucha para EPOLLIN
// (es decir, "avisame cuando haya una conexion nueva por aceptar").
void ChatServer::setup_epoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) throw std::runtime_error("epoll_create1()");

    epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) < 0)
        throw std::runtime_error("epoll_ctl(server_fd)");
}

// Crea la sala por defecto "general" y carga su historial desde disco (si existe).
void ChatServer::registrar_sala_default() {
    salas_.emplace("general", Sala{"general"});
    cargar_historial(salas_.at("general"));
}

// ── Bucle principal ──────────────────────────────────────────────────────────

// Espera eventos con epoll y los reparte. timeout de 500 ms para revisar de vez
// en cuando la bandera 'corriendo_' aunque no llegue nada.
void ChatServer::run() {
    epoll_event eventos[MAX_EVENTS];

    while (corriendo_) {
        int n = epoll_wait(epoll_fd_, eventos, MAX_EVENTS, 500);
        if (n < 0) {
            if (errno == EINTR) continue;  // interrumpido por una senal: reintentar
            break;
        }
        for (int i = 0; i < n; ++i) {
            int fd = eventos[i].data.fd;
            if (fd == server_fd_) {
                aceptar_conexion();                       // evento en el socket de escucha
            } else {
                // Evento en un cliente: error/cierre tiene prioridad.
                if (eventos[i].events & (EPOLLERR | EPOLLHUP)) {
                    desconectar_cliente(fd);
                } else if (eventos[i].events & EPOLLIN) {
                    leer_de_cliente(fd);                  // hay datos para leer
                } else if (eventos[i].events & EPOLLOUT) {
                    escribir_a_cliente(fd);               // el socket acepta escritura
                }
            }
        }
    }
    std::cout << "\n[Servidor] Cerrando...\n";
}

// ── Aceptar conexion ─────────────────────────────────────────────────────────

// Acepta TODAS las conexiones pendientes (bucle hasta EAGAIN, por edge-trigger).
// Por cada una: la pone non-blocking, la registra en epoll y le manda la
// bienvenida con la lista de salas.
void ChatServer::aceptar_conexion() {
    while (true) {
        sockaddr_in addr{};
        socklen_t len = sizeof(addr);
        int fd = accept(server_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        if (fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // no hay mas, salir
            std::cerr << "[Error] accept: " << strerror(errno) << "\n";
            break;
        }
        set_nonblocking(fd);

        // Registrar en epoll en modo edge-triggered para lectura.
        epoll_event ev{};
        ev.events  = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);

        // Crear la ficha del cliente (aun sin nombre ni sala).
        clientes_.emplace(fd, Cliente{fd, "", "", "", {}});

        // Saludo inicial + estado de salas para que el cliente ya pueda elegir.
        enviar(fd, proto::bienvenida("Bienvenido al Chat Server. Regístrate con {\"tipo\":\"unirse\",\"sala\":\"general\",\"usuario\":\"TuNombre\"}"));
        enviar(fd, proto::lista_salas(nombres_salas()));

        std::cout << "[+] Nueva conexión fd=" << fd
                  << " desde " << inet_ntoa(addr.sin_addr) << "\n";
    }
}

// ── Lectura ──────────────────────────────────────────────────────────────────

// Lee todo lo disponible del socket (bucle hasta EAGAIN). Acumula en el buffer
// del cliente y, cada vez que aparece un '\n', extrae ese mensaje, lo parsea
// como JSON y lo procesa. Si el JSON esta mal formado, avisa con un error.
void ChatServer::leer_de_cliente(int fd) {
    char buf[READ_BUF];
    while (true) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            // n == 0  -> el cliente cerro.
            // n < 0   -> error real (distinto de EAGAIN) -> desconectar.
            if (n == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
                desconectar_cliente(fd);
            break;
        }
        auto it = clientes_.find(fd);
        if (it == clientes_.end()) break;

        it->second.buffer_entrada.append(buf, static_cast<size_t>(n));

        // Extraer y procesar cada mensaje completo (terminado en '\n').
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

// Se llama cuando epoll avisa EPOLLOUT: el socket que antes estaba lleno ya
// admite datos. Vacia la cola de salida pendiente. Cuando queda vacia, deja de
// pedir EPOLLOUT (para no recibir avisos inutiles).
void ChatServer::escribir_a_cliente(int fd) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    auto& cola = it->second.cola_salida;
    while (!cola.empty()) {
        const std::string& msg = cola.front();
        ssize_t enviado = send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
        if (enviado < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break; // se lleno otra vez
            desconectar_cliente(fd);
            return;
        }
        cola.pop_front();
    }
    if (cola.empty()) epoll_mod_out(fd, false);
}

// Activa (activar=true) o desactiva el interes en EPOLLOUT para un fd, sin
// perder EPOLLIN ni el modo edge-triggered.
void ChatServer::epoll_mod_out(int fd, bool activar) {
    epoll_event ev{};
    ev.data.fd = fd;
    ev.events  = EPOLLIN | EPOLLET | (activar ? static_cast<uint32_t>(EPOLLOUT) : 0u);
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
}

// ── Desconexion ──────────────────────────────────────────────────────────────

// Saca al cliente de su sala (avisando al resto), lo quita de epoll, cierra el
// socket y borra su ficha. Seguro de llamar varias veces (comprueba existencia).
void ChatServer::desconectar_cliente(int fd) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    const std::string& nombre    = it->second.nombre;
    const std::string& sala_name = it->second.sala_actual;

    // Si estaba en una sala y ya tenia nombre, avisar "salio de la sala".
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

// Mira el campo "tipo" y llama al comando correspondiente. Si falta "tipo" o es
// desconocido, responde con un error.
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

// "unirse": registra el nombre del cliente y lo mete en la sala pedida (la crea
// si no existe). Valida campos, evita nombres duplicados, lo saca de su sala
// anterior, le manda el historial + estado y avisa al resto que entro.
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

    // No permitir dos clientes con el mismo nombre.
    for (auto& [ofd, c] : clientes_) {
        if (ofd != fd && c.nombre == nombre) {
            enviar(fd, proto::error_msg("El nombre '" + nombre + "' ya está en uso"));
            return;
        }
    }

    // Si ya estaba en otra sala, salir de ella y avisar.
    auto& cliente = clientes_.at(fd);
    if (!cliente.sala_actual.empty() && salas_.count(cliente.sala_actual)) {
        salas_.at(cliente.sala_actual).quitar_miembro(fd);
        broadcast_sala(cliente.sala_actual,
            proto::sistema(cliente.sala_actual, nombre + " cambió de sala"), fd);
    }

    // Crear la sala destino si es nueva (cargando su log).
    if (!salas_.count(sala_n)) {
        salas_.emplace(sala_n, Sala{sala_n});
        cargar_historial(salas_.at(sala_n));
    }

    cliente.nombre      = nombre;
    cliente.sala_actual = sala_n;
    salas_.at(sala_n).agregar_miembro(fd);

    // Enviar el historial reciente de la sala (si lo hay).
    auto hist = salas_.at(sala_n).ultimos_mensajes(HISTORIAL_ENVIAR);
    if (!hist.empty())
        enviar(fd, proto::historial(sala_n, hist));

    // Estado actual: lista de salas + usuarios de la sala.
    enviar(fd, proto::lista_salas(nombres_salas()));
    enviar(fd, proto::lista_usuarios(sala_n, usuarios_en_sala(sala_n)));

    // Avisar a los demas miembros que esta persona entro.
    broadcast_sala(sala_n,
        proto::sistema(sala_n, nombre + " se unió a la sala"), fd);

    std::cout << "[*] " << nombre << " se unió a #" << sala_n << "\n";
}

// "crear_sala": registra una sala nueva (sin meter al cliente en ella) y avisa
// a TODOS los clientes con la lista de salas actualizada.
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

    // Notificar la nueva sala a todos los conectados.
    json notif = proto::lista_salas(nombres_salas());
    for (auto& [ofd, _] : clientes_) enviar(ofd, notif);

    std::cout << "[*] " << nombre << " creó la sala #" << sala_n << "\n";
}

// "mensaje": texto de un usuario. Lo guarda en historial (memoria + log) y lo
// reenvia a toda la sala, incluido el remitente (eco de confirmacion).
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

    // Persistencia: en memoria (para historial al entrar) y en archivo de log.
    salas_.at(sala_n).guardar_en_historial(bcast);
    guardar_mensaje_log(sala_n, bcast);

    // Reenvio a todos (sin excluir a nadie -> el remitente ve su propio mensaje).
    broadcast_sala(sala_n, bcast);
}

// "imagen": igual que un mensaje, pero el payload es la imagen en base64. Se
// valida un tamano maximo, se reenvia completa a la sala y en el historial se
// guarda SOLO una nota ligera (no los megabytes) para no inflar el log.
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

    // Limite de seguridad: ~3MB de base64 (~2.2MB de imagen real).
    if (datos.size() > 3u * 1024 * 1024) {
        enviar(fd, proto::error_msg("Imagen demasiado grande (max ~2MB)"));
        return;
    }

    json img = proto::imagen(sala_n, nombre, archivo, datos);

    // Reenviar la imagen completa a todos los miembros (incluido el remitente).
    broadcast_sala(sala_n, img);

    // Historial: solo una nota, no el payload pesado.
    json nota = proto::sistema(sala_n, nombre + " compartio la imagen: " + archivo);
    salas_.at(sala_n).guardar_en_historial(nota);
    guardar_mensaje_log(sala_n, nota);

    std::cout << "[img] " << nombre << " compartio " << archivo
              << " (" << datos.size() << " bytes b64) en #" << sala_n << "\n";
}

// "listar_salas": responde con todas las salas existentes.
void ChatServer::cmd_listar_salas(int fd) {
    enviar(fd, proto::lista_salas(nombres_salas()));
}

// "listar_usuarios": responde con los usuarios de la sala pedida (o, si no se
// indica, la sala actual del que pregunta).
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

// "salir": el cliente pide desconectarse; se reutiliza la limpieza normal.
void ChatServer::cmd_salir(int fd) {
    desconectar_cliente(fd);
}

// ── Envio ────────────────────────────────────────────────────────────────────

// Envia un JSON a un cliente sin bloquear. Estrategia:
//   - Si su cola esta vacia, intenta enviar directo con send().
//   - Si se envia completo, listo.
//   - Si se envia parcial o da EAGAIN (socket lleno), guarda lo que falta en la
//     cola y activa EPOLLOUT para terminar luego en escribir_a_cliente().
//   - Si la cola ya tenia cosas, simplemente agrega al final (preserva el orden).
void ChatServer::enviar(int fd, const json& msg) {
    auto it = clientes_.find(fd);
    if (it == clientes_.end()) return;

    std::string serializado = proto::serialize(msg);
    auto& cola = it->second.cola_salida;

    if (cola.empty()) {
        ssize_t n = send(fd, serializado.c_str(), serializado.size(), MSG_NOSIGNAL);
        if (n == static_cast<ssize_t>(serializado.size())) return; // enviado entero
        if (n > 0) serializado = serializado.substr(static_cast<size_t>(n)); // quedo a medias
        if (errno == EAGAIN || errno == EWOULDBLOCK || n > 0) {
            cola.push_back(std::move(serializado));
            epoll_mod_out(fd, true);
            return;
        }
        desconectar_cliente(fd); // error real al enviar
        return;
    }
    cola.push_back(std::move(serializado));
    epoll_mod_out(fd, true);
}

// Reenvia un mensaje a todos los miembros de una sala. 'excluir_fd' (por defecto
// -1) permite NO mandarselo a alguien (p. ej. al que provoco el aviso).
void ChatServer::broadcast_sala(const std::string& sala, const json& msg, int excluir_fd) {
    if (!salas_.count(sala)) return;
    for (int fd : salas_.at(sala).miembros_fd) {
        if (fd != excluir_fd) enviar(fd, msg);
    }
}

// ── Historial (persistencia en archivos) ─────────────────────────────────────

// Ruta del archivo de log de una sala: "logs/<sala>.log".
std::string ChatServer::ruta_log(const std::string& sala) const {
    return "logs/" + sala + ".log";
}

// Carga el historial de disco a memoria al crear/levantar una sala. Cada linea
// del archivo es un mensaje JSON (formato JSON Lines). Lineas corruptas se omiten.
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

// Anexa (append) un mensaje al final del log de la sala, una linea por mensaje.
void ChatServer::guardar_mensaje_log(const std::string& sala, const json& msg) {
    std::ofstream f(ruta_log(sala), std::ios::app);
    if (f.is_open()) f << msg.dump() << "\n";
}

// ── Utilidades ───────────────────────────────────────────────────────────────

// Devuelve los nombres de todas las salas existentes.
std::vector<std::string> ChatServer::nombres_salas() const {
    std::vector<std::string> v;
    v.reserve(salas_.size());
    for (auto& [k, _] : salas_) v.push_back(k);
    return v;
}

// Devuelve los nombres de los usuarios presentes (y ya registrados) en una sala.
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
