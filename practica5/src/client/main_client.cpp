// ============================================================================
//  main_client.cpp
//  ----------------------------------------------------------------------------
//  Punto de entrada del CLIENTE. Valida argumentos, conecta al servidor, crea
//  la TUI y entra en la sala "general".
//
//  Uso:  ./client <host> <puerto> <usuario>
//  Ej.:  ./client 127.0.0.1 8080 Alice
// ============================================================================
#include <iostream>
#include <cstdlib>
#include "chat_client.hpp"
#include "tui.hpp"

// Mensaje de ayuda cuando faltan argumentos.
static void usage(const char* prog) {
    std::cerr << "Uso: " << prog << " <host> <puerto> <usuario>\n"
              << "  Ejemplo: " << prog << " 127.0.0.1 8080 Alice\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) { usage(argv[0]); return 1; }

    std::string host    = argv[1];
    int         puerto  = std::atoi(argv[2]);
    std::string usuario = argv[3];

    // Validaciones basicas de los argumentos.
    if (puerto <= 0 || puerto > 65535) {
        std::cerr << "Puerto inválido\n";
        return 1;
    }
    if (usuario.empty()) {
        std::cerr << "El nombre de usuario no puede estar vacío\n";
        return 1;
    }

    // Conectar antes de arrancar la TUI (si falla, mostramos error en consola).
    ChatClient cliente(host, puerto, usuario);
    if (!cliente.conectar()) {
        std::cerr << "No se pudo conectar a " << host << ":" << puerto << "\n";
        return 1;
    }

    // A partir de aqui la pantalla pertenece a ncurses.
    TUI tui(cliente);

    // Entrar automaticamente a la sala "general" al iniciar.
    cliente.set_sala_actual("general");
    cliente.enviar_unirse("general");

    tui.run();   // bucle hasta que el usuario salga o se caiga la conexion
    return 0;
}
