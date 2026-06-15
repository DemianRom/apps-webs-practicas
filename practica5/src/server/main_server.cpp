// ============================================================================
//  main_server.cpp
//  ----------------------------------------------------------------------------
//  Punto de entrada del SERVIDOR. Lee el puerto de la linea de comandos
//  (por defecto 8080), construye el ChatServer y arranca su bucle.
//
//  Uso:  ./server [puerto]
// ============================================================================
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "chat_server.hpp"

int main(int argc, char* argv[]) {
    int puerto = 8080; // valor por defecto si no se pasa argumento

    // Argumento opcional: el puerto. Se valida que este en rango (1..65535).
    if (argc >= 2) {
        puerto = std::atoi(argv[1]);
        if (puerto <= 0 || puerto > 65535) {
            std::cerr << "Puerto inválido: " << argv[1] << "\n";
            return 1;
        }
    }

    // Construir y correr el servidor. Cualquier fallo de setup llega como
    // excepcion y se reporta aqui en vez de abortar sin explicacion.
    try {
        ChatServer srv(puerto);
        srv.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
