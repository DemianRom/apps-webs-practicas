#pragma once
#include <ncurses.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include "chat_client.hpp"

struct MensajeTUI {
    std::string texto;
    int color_pair; // 1=normal, 2=sistema, 3=propio, 4=error
};

class TUI {
public:
    explicit TUI(ChatClient& cliente);
    ~TUI();

    void run();

private:
    ChatClient& cliente_;
    std::mutex  mutex_;

    // Ventanas ncurses
    WINDOW* win_salas_   = nullptr; // panel izquierdo: salas
    WINDOW* win_usuarios_= nullptr; // panel izquierdo: usuarios
    WINDOW* win_chat_    = nullptr; // panel central: mensajes
    WINDOW* win_status_  = nullptr; // barra de estado superior
    WINDOW* win_input_   = nullptr; // línea de entrada inferior

    // Estado
    std::deque<MensajeTUI>   mensajes_;
    std::vector<std::string> salas_;
    std::vector<std::string> usuarios_;
    std::string              sala_activa_;
    std::string              input_buffer_;
    bool                     corriendo_ = true;
    int                      sala_idx_  = 0;  // índice seleccionado en lista de salas

    static constexpr int MAX_MSGS = 500;
    static constexpr int PANEL_W  = 20;       // ancho panel izquierdo

    // Setup
    void init_ncurses();
    void crear_ventanas();
    void limpiar();

    // Render
    void redraw_all();
    void draw_status();
    void draw_salas();
    void draw_usuarios();
    void draw_chat();
    void draw_input();

    // Callbacks del cliente
    void on_mensaje(const json& msg);
    void agregar_mensaje(const std::string& texto, int color);

    // Comandos internos
    void procesar_input();
    void mostrar_ayuda();
    void cambiar_sala(const std::string& sala);
};
