#pragma once
// ============================================================================
//  protocolo.hpp
//  ----------------------------------------------------------------------------
//  Definicion del protocolo de mensajes del chat. Cliente y servidor comparten
//  este archivo, de modo que ambos usan exactamente los mismos nombres de tipo
//  ("unirse", "mensaje", ...) y la misma forma de construir/serializar el JSON.
//
//  Idea general:
//   - Todos los mensajes son objetos JSON con un campo obligatorio "tipo".
//   - Cada mensaje viaja por TCP como UNA linea de texto terminada en '\n'
//     (framing por salto de linea). Ver proto::serialize().
// ============================================================================
#include "json.hpp"
#include <string>
#include <vector>

// Alias corto: en todo el proyecto escribimos 'json' en vez de 'nlohmann::json'.
using json = nlohmann::json;

// ── Tipos de mensajes CLIENTE → SERVIDOR ────────────────────────────────────
inline constexpr const char* T_UNIRSE        = "unirse";          // registrarse/entrar a una sala
inline constexpr const char* T_CREAR_SALA    = "crear_sala";      // crear una sala nueva
inline constexpr const char* T_MENSAJE       = "mensaje";         // enviar texto a la sala
inline constexpr const char* T_IMAGEN        = "imagen";          // enviar imagen (base64) a la sala
inline constexpr const char* T_LISTAR_SALAS  = "listar_salas";    // pedir la lista de salas
inline constexpr const char* T_LISTAR_USERS  = "listar_usuarios"; // pedir los usuarios de una sala
inline constexpr const char* T_SALIR         = "salir";           // desconectarse limpiamente

// ── Tipos de mensajes SERVIDOR → CLIENTE ────────────────────────────────────
inline constexpr const char* T_LISTA_SALAS   = "lista_salas";     // respuesta con todas las salas
inline constexpr const char* T_LISTA_USERS   = "lista_usuarios";  // respuesta con usuarios de una sala
inline constexpr const char* T_BROADCAST     = "broadcast";       // mensaje de un usuario reenviado
inline constexpr const char* T_SISTEMA       = "sistema";         // aviso del sistema (entradas/salidas)
inline constexpr const char* T_ERROR         = "error";           // descripcion de un error
inline constexpr const char* T_BIENVENIDA    = "bienvenida";      // saludo inicial al conectar
inline constexpr const char* T_HISTORIAL     = "historial";       // ultimos mensajes al entrar a sala

// ── Helpers de construccion ─────────────────────────────────────────────────
// Estas funciones solo arman el objeto JSON con los campos correctos. Asi se
// evita escribir los nombres de campo a mano (y equivocarse) en cada sitio.
namespace proto {

// Convierte un JSON a la cadena que se manda por el socket: el dump compacto
// MAS el '\n' final que marca el fin del mensaje (framing).
inline std::string serialize(const json& j) {
    return j.dump() + "\n";
}

// { "tipo":"lista_salas", "salas":[...] }
inline json lista_salas(const std::vector<std::string>& salas) {
    return { {"tipo", T_LISTA_SALAS}, {"salas", salas} };
}

// { "tipo":"lista_usuarios", "sala":..., "usuarios":[...] }
inline json lista_usuarios(const std::string& sala, const std::vector<std::string>& usuarios) {
    return { {"tipo", T_LISTA_USERS}, {"sala", sala}, {"usuarios", usuarios} };
}

// { "tipo":"broadcast", "sala":..., "usuario":..., "contenido":... }
// Es el mensaje de un usuario tal como lo reciben los demas (y el mismo, como eco).
inline json broadcast(const std::string& sala, const std::string& usuario, const std::string& contenido) {
    return { {"tipo", T_BROADCAST}, {"sala", sala}, {"usuario", usuario}, {"contenido", contenido} };
}

// { "tipo":"imagen", "sala":..., "usuario":..., "nombre_archivo":..., "datos":<base64> }
// 'datos' es la imagen completa codificada en base64 (texto seguro para JSON).
inline json imagen(const std::string& sala, const std::string& usuario,
                   const std::string& nombre_archivo, const std::string& datos_b64) {
    return { {"tipo", T_IMAGEN}, {"sala", sala}, {"usuario", usuario},
             {"nombre_archivo", nombre_archivo}, {"datos", datos_b64} };
}

// { "tipo":"sistema", "sala":..., "contenido":... }
// Aviso automatico (ej. "Pepe se unio a la sala"). No tiene "usuario".
inline json sistema(const std::string& sala, const std::string& msg) {
    return { {"tipo", T_SISTEMA}, {"sala", sala}, {"contenido", msg} };
}

// { "tipo":"error", "mensaje":... }
inline json error_msg(const std::string& msg) {
    return { {"tipo", T_ERROR}, {"mensaje", msg} };
}

// { "tipo":"bienvenida", "contenido":... }
inline json bienvenida(const std::string& msg) {
    return { {"tipo", T_BIENVENIDA}, {"contenido", msg} };
}

// { "tipo":"historial", "sala":..., "mensajes":[ ...mensajes previos... ] }
// Se envia al entrar a una sala para mostrar lo que se dijo antes.
inline json historial(const std::string& sala, const std::vector<json>& mensajes) {
    return { {"tipo", T_HISTORIAL}, {"sala", sala}, {"mensajes", mensajes} };
}

} // namespace proto
