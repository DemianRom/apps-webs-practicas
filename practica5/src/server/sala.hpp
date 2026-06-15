#pragma once
#include <string>
#include <unordered_set>
#include <deque>
#include "../common/protocolo.hpp"

inline constexpr size_t MAX_HISTORIAL = 50;

struct Sala {
    std::string nombre;
    std::unordered_set<int> miembros_fd;
    std::deque<json> historial; // últimos MAX_HISTORIAL mensajes

    explicit Sala(std::string n) : nombre(std::move(n)) {}

    void agregar_miembro(int fd)    { miembros_fd.insert(fd); }
    void quitar_miembro(int fd)     { miembros_fd.erase(fd); }
    bool tiene_miembro(int fd) const { return miembros_fd.count(fd) > 0; }
    bool vacia() const              { return miembros_fd.empty(); }

    // Guarda un mensaje en el historial en memoria (el archivo lo maneja ChatServer)
    void guardar_en_historial(const json& msg) {
        if (historial.size() >= MAX_HISTORIAL)
            historial.pop_front();
        historial.push_back(msg);
    }

    std::vector<json> ultimos_mensajes(size_t n) const {
        size_t start = historial.size() > n ? historial.size() - n : 0;
        return { historial.begin() + start, historial.end() };
    }
};
