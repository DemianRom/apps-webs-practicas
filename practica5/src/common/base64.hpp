#pragma once
// ============================================================================
//  base64.hpp
//  ----------------------------------------------------------------------------
//  Codificacion y decodificacion Base64 (RFC 4648), sin dependencias externas.
//
//  ¿Para que? El JSON solo puede transportar texto valido, pero una imagen son
//  bytes binarios arbitrarios. Base64 convierte esos bytes en texto ASCII
//  seguro (A-Z a-z 0-9 + /), de modo que se pueden meter dentro del campo
//  "datos" de un mensaje JSON. Al recibirlo, se decodifica de vuelta a bytes.
//
//  Cada 3 bytes (24 bits) se reescriben como 4 caracteres de 6 bits. Si los
//  bytes no son multiplo de 3, se rellena con '=' al final.
// ============================================================================
#include <string>
#include <vector>
#include <cstdint>

namespace b64 {

// Alfabeto Base64: el indice (0..63) da el caracter correspondiente.
inline const char* tabla() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

// Codifica un bloque de bytes a una cadena Base64.
//   datos : bytes de entrada (p. ej. el contenido de un archivo de imagen).
//   return: el texto Base64 equivalente.
inline std::string encode(const std::vector<uint8_t>& datos) {
    const char* T = tabla();
    std::string out;
    out.reserve(((datos.size() + 2) / 3) * 4); // tamano final aproximado

    // Procesar de 3 bytes en 3 bytes -> 4 caracteres.
    size_t i = 0;
    while (i + 3 <= datos.size()) {
        uint32_t n = (datos[i] << 16) | (datos[i + 1] << 8) | datos[i + 2];
        out.push_back(T[(n >> 18) & 0x3F]);
        out.push_back(T[(n >> 12) & 0x3F]);
        out.push_back(T[(n >> 6)  & 0x3F]);
        out.push_back(T[n & 0x3F]);
        i += 3;
    }
    // Bytes sobrantes (1 o 2): se completan con relleno '='.
    size_t resto = datos.size() - i;
    if (resto == 1) {
        uint32_t n = datos[i] << 16;
        out.push_back(T[(n >> 18) & 0x3F]);
        out.push_back(T[(n >> 12) & 0x3F]);
        out.push_back('=');
        out.push_back('=');
    } else if (resto == 2) {
        uint32_t n = (datos[i] << 16) | (datos[i + 1] << 8);
        out.push_back(T[(n >> 18) & 0x3F]);
        out.push_back(T[(n >> 12) & 0x3F]);
        out.push_back(T[(n >> 6)  & 0x3F]);
        out.push_back('=');
    }
    return out;
}

// Operacion inversa de tabla(): dado un caracter Base64 devuelve su valor 0..63,
// o -1 si es relleno '=' o un caracter que hay que ignorar (ej. salto de linea).
inline int valor(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

// Decodifica una cadena Base64 de vuelta a sus bytes originales.
//   in    : texto Base64.
//   return: los bytes reconstruidos.
// Tecnica: se acumulan bits de 6 en 6 y, cada vez que hay >= 8, se emite 1 byte.
inline std::vector<uint8_t> decode(const std::string& in) {
    std::vector<uint8_t> out;
    out.reserve((in.size() / 4) * 3);

    int acc = 0, bits = 0;
    for (char c : in) {
        if (c == '=') break;        // relleno: fin de datos utiles
        int v = valor(c);
        if (v < 0) continue;        // ignorar caracteres no validos (\n, espacios)
        acc = (acc << 6) | v;
        bits += 6;
        if (bits >= 8) {
            bits -= 8;
            out.push_back(static_cast<uint8_t>((acc >> bits) & 0xFF));
        }
    }
    return out;
}

} // namespace b64
