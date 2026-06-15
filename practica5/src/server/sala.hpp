#pragma once
// ============================================================================
//  sala.hpp
//  ----------------------------------------------------------------------------
//  Una sala de chat: su nombre, quienes estan dentro (por descriptor de socket)
//  y un historial corto de los ultimos mensajes guardado en memoria.
//  El servidor mantiene un mapa nombre -> Sala.
// ============================================================================
#include <string>
#include <unordered_set>
#include <deque>
#include "../common/protocolo.hpp"

// Cuantos mensajes recientes conserva cada sala en memoria (RAM).
// El archivo de log en disco puede ser mas largo; esto es solo la ventana viva.
inline constexpr size_t MAX_HISTORIAL = 50;

struct Sala {
    std::string nombre;
    std::unordered_set<int> miembros_fd;  // fds de los clientes presentes en la sala
    std::deque<json> historial;           // ultimos <= MAX_HISTORIAL mensajes (en memoria)

    explicit Sala(std::string n) : nombre(std::move(n)) {}

    // Gestion de miembros (entrar/salir/consultar/esta vacia).
    void agregar_miembro(int fd)     { miembros_fd.insert(fd); }
    void quitar_miembro(int fd)      { miembros_fd.erase(fd); }
    bool tiene_miembro(int fd) const { return miembros_fd.count(fd) > 0; }
    bool vacia() const               { return miembros_fd.empty(); }

    // Agrega un mensaje al historial en memoria. Si ya hay MAX_HISTORIAL,
    // descarta el mas antiguo (cola circular de tamano acotado).
    // El guardado en archivo lo hace ChatServer aparte.
    void guardar_en_historial(const json& msg) {
        if (historial.size() >= MAX_HISTORIAL)
            historial.pop_front();
        historial.push_back(msg);
    }

    // Devuelve una copia de los ultimos 'n' mensajes (o todos si hay menos).
    // Se usa para enviar el historial a quien acaba de entrar a la sala.
    std::vector<json> ultimos_mensajes(size_t n) const {
        size_t start = historial.size() > n ? historial.size() - n : 0;
        return { historial.begin() + start, historial.end() };
    }
};
