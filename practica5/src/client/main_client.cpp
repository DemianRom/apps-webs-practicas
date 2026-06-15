#include <iostream>
#include <cstdlib>
#include "chat_client.hpp"
#include "tui.hpp"

static void usage(const char* prog) {
    std::cerr << "Uso: " << prog << " <host> <puerto> <usuario>\n"
              << "  Ejemplo: " << prog << " 127.0.0.1 8080 Alice\n";
}

int main(int argc, char* argv[]) {
    if (argc < 4) { usage(argv[0]); return 1; }

    std::string host    = argv[1];
    int         puerto  = std::atoi(argv[2]);
    std::string usuario = argv[3];

    if (puerto <= 0 || puerto > 65535) {
        std::cerr << "Puerto inválido\n";
        return 1;
    }
    if (usuario.empty()) {
        std::cerr << "El nombre de usuario no puede estar vacío\n";
        return 1;
    }

    ChatClient cliente(host, puerto, usuario);
    if (!cliente.conectar()) {
        std::cerr << "No se pudo conectar a " << host << ":" << puerto << "\n";
        return 1;
    }

    TUI tui(cliente);

    // Unirse a la sala general al iniciar
    cliente.set_sala_actual("general");
    cliente.enviar_unirse("general");

    tui.run();
    return 0;
}
