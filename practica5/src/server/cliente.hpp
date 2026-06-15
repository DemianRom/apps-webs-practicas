#pragma once
#include <string>
#include <deque>

struct Cliente {
    int  fd;
    std::string nombre;
    std::string sala_actual;
    std::string buffer_entrada;   // acumulador de bytes leídos (framing por \n)
    std::deque<std::string> cola_salida; // mensajes pendientes de escribir

    bool registrado() const { return !nombre.empty(); }
};
