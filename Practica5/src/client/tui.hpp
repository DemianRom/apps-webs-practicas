/*
 * Practica 5 - Servidor de chat no bloqueante
 * Materia: Aplicaciones y Comunicaciones en Red (6CM1)
 * ESCOM - IPN | Ingenieria en Sistemas Computacionales (6to semestre)
 * Periodo: 26/2
 *
 * Integrantes:
 * - Romero Bautista Demian
 * - Ferreira Rodriguez Said
 *
 * Responsabilidad del archivo:
 * Declara la interfaz TUI y el estado visual del cliente de chat.
 */

#pragma once
// ============================================================================
//  tui.hpp
//  ----------------------------------------------------------------------------
//  Interfaz de usuario en terminal (TUI) con ncurses. Dibuja cuatro zonas:
//    ┌───────────── status (barra superior) ─────────────┐
//    │ Salas    │                                        │
//    │──────────│            Chat (mensajes)             │
//    │ Usuarios │                                        │
//    └──────────┴────────────────────────────────────────┘
//    > input (linea inferior)
//
//  La TUI corre en el hilo principal; el ChatClient le entrega los mensajes del
//  servidor desde OTRO hilo via callback (on_mensaje). Por eso el estado
//  compartido (mensajes_, salas_, usuarios_, ...) se protege con mutex_.
// ============================================================================
#include <ncurses.h>
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include "chat_client.hpp"

// Una linea ya lista para pintar en el panel de chat.
struct MensajeTUI {
    std::string texto;
    int  color_pair;        // par de color ncurses (1=normal,2=sistema,3=propio,4=error,9=imagen)
    bool destacado = false; // true si este mensaje me menciona (@miusuario)
    int  img_idx   = -1;    // si es una imagen recibida, su indice en imagenes_
};

// Metadatos de una imagen recibida y ya decodificada a disco en esta sesion.
struct ImagenRecibida {
    std::string nombre;     // nombre original del archivo
    std::string ruta_local; // ruta temporal donde se guardo (/tmp/chat_p5/...)
    std::string de;         // usuario que la envio
};

class TUI {
public:
    explicit TUI(ChatClient& cliente); // inicializa ncurses y crea las ventanas
    ~TUI();                            // restaura la terminal

    // Bucle principal: registra el callback, lee teclado y redibuja hasta salir.
    void run();

private:
    ChatClient& cliente_;
    std::mutex  mutex_;     // protege el estado compartido con el hilo de recepcion

    // Ventanas ncurses (cada panel es una WINDOW independiente).
    WINDOW* win_salas_   = nullptr; // panel izquierdo arriba: salas
    WINDOW* win_usuarios_= nullptr; // panel izquierdo abajo: usuarios
    WINDOW* win_chat_    = nullptr; // panel central: mensajes
    WINDOW* win_status_  = nullptr; // barra de estado superior
    WINDOW* win_input_   = nullptr; // linea de entrada inferior

    // ── Estado de la interfaz ────────────────────────────────────────────────
    std::deque<MensajeTUI>       mensajes_;    // historial visible del chat
    std::vector<std::string>     salas_;       // salas conocidas
    std::vector<std::string>     usuarios_;    // usuarios de la sala activa
    std::vector<ImagenRecibida>  imagenes_;    // imagenes decodificadas en la sesion
    std::string                  sala_activa_; // sala en la que estoy
    std::string                  input_buffer_;// texto que estoy escribiendo
    bool                         corriendo_ = true; // sigue el bucle de la TUI
    int                          sala_idx_  = 0; // sala resaltada (para Tab)

    static constexpr int MAX_MSGS = 500;  // tope de lineas guardadas en memoria
    static constexpr int PANEL_W  = 20;   // ancho del panel izquierdo

    // ── Setup / teardown de ncurses ──────────────────────────────────────────
    void init_ncurses();   // modos, colores
    void crear_ventanas(); // calcula tamanos y crea las WINDOW
    void limpiar();        // libera ventanas y cierra ncurses

    // ── Render (cada funcion pinta una zona) ─────────────────────────────────
    void redraw_all();   // redibuja todo bajo el mutex
    void draw_status();
    void draw_salas();
    void draw_usuarios();
    void draw_chat();
    void draw_input();

    // ── Entrada de mensajes del servidor ─────────────────────────────────────
    void on_mensaje(const json& msg);                 // callback (otro hilo)
    void agregar_mensaje(const std::string& texto, int color); // push acotado
    void recibir_imagen(const json& msg);             // decodifica y marca imagen

    // ── Comandos de la propia interfaz ───────────────────────────────────────
    void procesar_input();                    // interpreta lo escrito (texto o /comando)
    void mostrar_ayuda();                     // imprime /help
    void cambiar_sala(const std::string& sala);
    void ver_imagen(int n);                   // muestra imagen n (Kitty / visor)

    // ── Helpers ──────────────────────────────────────────────────────────────
    bool me_mencionan(const std::string& texto) const;       // ¿aparece @miusuario?
    void draw_linea_chat(int y, int cols, const MensajeTUI& m); // pinta una linea
};
