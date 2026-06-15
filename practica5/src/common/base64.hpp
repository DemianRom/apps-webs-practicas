#pragma once
#include <string>
#include <vector>
#include <cstdint>

// Codificacion/decodificacion base64 (RFC 4648), sin dependencias externas.
namespace b64 {

inline const char* tabla() {
    return "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
}

inline std::string encode(const std::vector<uint8_t>& datos) {
    const char* T = tabla();
    std::string out;
    out.reserve(((datos.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 3 <= datos.size()) {
        uint32_t n = (datos[i] << 16) | (datos[i + 1] << 8) | datos[i + 2];
        out.push_back(T[(n >> 18) & 0x3F]);
        out.push_back(T[(n >> 12) & 0x3F]);
        out.push_back(T[(n >> 6)  & 0x3F]);
        out.push_back(T[n & 0x3F]);
        i += 3;
    }
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

inline int valor(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1; // '=' o caracter invalido
}

inline std::vector<uint8_t> decode(const std::string& in) {
    std::vector<uint8_t> out;
    out.reserve((in.size() / 4) * 3);

    int acc = 0, bits = 0;
    for (char c : in) {
        if (c == '=' ) break;
        int v = valor(c);
        if (v < 0) continue; // ignorar saltos de linea u otros
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
