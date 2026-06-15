#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include "chat_server.hpp"

int main(int argc, char* argv[]) {
    int puerto = 8080;
    if (argc >= 2) {
        puerto = std::atoi(argv[1]);
        if (puerto <= 0 || puerto > 65535) {
            std::cerr << "Puerto inválido: " << argv[1] << "\n";
            return 1;
        }
    }
    try {
        ChatServer srv(puerto);
        srv.run();
    } catch (const std::exception& e) {
        std::cerr << "[Fatal] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
