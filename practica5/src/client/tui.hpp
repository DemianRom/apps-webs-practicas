#pragma once
#include <ncurses.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include "chat_client.hpp"

struct MensajeTUI {
    std::string texto;
    int  color_pair;       // 1=normal, 2=sistema, 3=propio, 4=error, 5=imagen
    bool destacado = false; // true si me mencionan con @
    int  img_idx   = -1;    // indice en imagenes_ si es una imagen recibida
};

struct ImagenRecibida {
    std::string nombre;     // nombre original del archivo
    std::string ruta_local; // ruta temporal donde se decodifico
    std::string de;         // usuario que la envio
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
    std::deque<MensajeTUI>       mensajes_;
    std::vector<std::string>     salas_;
    std::vector<std::string>     usuarios_;
    std::vector<ImagenRecibida>  imagenes_;   // imagenes decodificadas en esta sesion
    std::string                  sala_activa_;
    std::string                  input_buffer_;
    bool                         corriendo_ = true;
    int                          sala_idx_  = 0;  // índice seleccionado en lista de salas

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
    void recibir_imagen(const json& msg);

    // Comandos internos
    void procesar_input();
    void mostrar_ayuda();
    void cambiar_sala(const std::string& sala);
    void ver_imagen(int n);

    // Helpers
    bool me_mencionan(const std::string& texto) const;
    void draw_linea_chat(int y, int cols, const MensajeTUI& m);
};
