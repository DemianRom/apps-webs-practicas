#pragma once
// ============================================================================
//  cliente.hpp
//  ----------------------------------------------------------------------------
//  Representa, del lado del SERVIDOR, a un cliente conectado por un socket.
//  No es la aplicacion del cliente (eso es chat_client.*), sino la "ficha" que
//  el servidor guarda por cada conexion: quien es, en que sala esta y los
//  buffers de datos de entrada/salida de su socket.
// ============================================================================
#include <string>
#include <deque>

struct Cliente {
    int  fd;                  // descriptor del socket de esta conexion
    std::string nombre;       // apodo del usuario (vacio = aun no se ha registrado)
    std::string sala_actual;  // sala en la que esta ahora mismo

    // Bytes ya leidos del socket pero aun no procesados. Como TCP es un flujo,
    // un recv() puede traer medio mensaje o varios; aqui se acumulan hasta
    // encontrar un '\n' que marque un mensaje completo.
    std::string buffer_entrada;

    // Mensajes que faltan por escribir al socket cuando este se llena
    // (send() devolvio EAGAIN). Se vacian cuando epoll avisa EPOLLOUT.
    std::deque<std::string> cola_salida;

    // ¿Ya envio un "unirse" con nombre valido? Sirve para rechazar mensajes
    // de quien todavia no se identifico.
    bool registrado() const { return !nombre.empty(); }
};
