#pragma once
#include "json.hpp"
#include <string>

using json = nlohmann::json;

// ── Tipos de mensajes cliente → servidor ────────────────────────────────────
inline constexpr const char* T_UNIRSE        = "unirse";
inline constexpr const char* T_CREAR_SALA    = "crear_sala";
inline constexpr const char* T_MENSAJE       = "mensaje";
inline constexpr const char* T_LISTAR_SALAS  = "listar_salas";
inline constexpr const char* T_LISTAR_USERS  = "listar_usuarios";
inline constexpr const char* T_SALIR         = "salir";

// ── Tipos de mensajes servidor → cliente ────────────────────────────────────
inline constexpr const char* T_LISTA_SALAS   = "lista_salas";
inline constexpr const char* T_LISTA_USERS   = "lista_usuarios";
inline constexpr const char* T_BROADCAST     = "broadcast";
inline constexpr const char* T_SISTEMA       = "sistema";
inline constexpr const char* T_ERROR         = "error";
inline constexpr const char* T_BIENVENIDA    = "bienvenida";
inline constexpr const char* T_HISTORIAL     = "historial";

// ── Helpers de construcción ──────────────────────────────────────────────────
namespace proto {

inline std::string serialize(const json& j) {
    return j.dump() + "\n";
}

inline json lista_salas(const std::vector<std::string>& salas) {
    return { {"tipo", T_LISTA_SALAS}, {"salas", salas} };
}

inline json lista_usuarios(const std::string& sala, const std::vector<std::string>& usuarios) {
    return { {"tipo", T_LISTA_USERS}, {"sala", sala}, {"usuarios", usuarios} };
}

inline json broadcast(const std::string& sala, const std::string& usuario, const std::string& contenido) {
    return { {"tipo", T_BROADCAST}, {"sala", sala}, {"usuario", usuario}, {"contenido", contenido} };
}

inline json sistema(const std::string& sala, const std::string& msg) {
    return { {"tipo", T_SISTEMA}, {"sala", sala}, {"contenido", msg} };
}

inline json error_msg(const std::string& msg) {
    return { {"tipo", T_ERROR}, {"mensaje", msg} };
}

inline json bienvenida(const std::string& msg) {
    return { {"tipo", T_BIENVENIDA}, {"contenido", msg} };
}

inline json historial(const std::string& sala, const std::vector<json>& mensajes) {
    return { {"tipo", T_HISTORIAL}, {"sala", sala}, {"mensajes", mensajes} };
}

} // namespace proto
