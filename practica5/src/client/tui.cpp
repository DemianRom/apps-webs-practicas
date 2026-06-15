#include "tui.hpp"

#include <algorithm>
#include <sstream>
#include <cstring>

// ── Constructor / Destructor ─────────────────────────────────────────────────

TUI::TUI(ChatClient& cliente) : cliente_(cliente) {
    init_ncurses();
    crear_ventanas();
}

TUI::~TUI() {
    limpiar();
}

void TUI::init_ncurses() {
    setlocale(LC_ALL, "");   // UTF-8 support
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(1, COLOR_WHITE,   -1);              // normal (mensajes)
        init_pair(2, COLOR_YELLOW,  -1);              // sistema / info
        init_pair(3, COLOR_CYAN,    -1);              // propio mensaje
        init_pair(4, COLOR_RED,     -1);              // error
        init_pair(5, COLOR_MAGENTA, -1);              // sala seleccionada
        init_pair(6, COLOR_WHITE,   -1);              // bordes / títulos
        init_pair(7, COLOR_BLACK,   COLOR_GREEN);     // status bar (negro sobre verde)
    }
}

void TUI::crear_ventanas() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int chat_w = cols - PANEL_W - 1;
    int panel_h_salas   = rows / 2 - 1;
    int panel_h_usuarios = rows - panel_h_salas - 3;

    win_status_   = newwin(1, cols, 0, 0);
    win_salas_    = newwin(panel_h_salas,    PANEL_W, 1, 0);
    win_usuarios_ = newwin(panel_h_usuarios, PANEL_W, 1 + panel_h_salas, 0);
    win_chat_     = newwin(rows - 3, chat_w, 1, PANEL_W + 1);
    win_input_    = newwin(2, cols, rows - 2, 0);
}

void TUI::limpiar() {
    if (win_salas_)    { delwin(win_salas_);    win_salas_    = nullptr; }
    if (win_usuarios_) { delwin(win_usuarios_); win_usuarios_ = nullptr; }
    if (win_chat_)     { delwin(win_chat_);     win_chat_     = nullptr; }
    if (win_status_)   { delwin(win_status_);   win_status_   = nullptr; }
    if (win_input_)    { delwin(win_input_);    win_input_    = nullptr; }
    endwin();
}

// ── Loop principal ───────────────────────────────────────────────────────────

void TUI::run() {
    // Registrar callback para mensajes del servidor
    cliente_.iniciar_recepcion([this](const json& msg) {
        on_mensaje(msg);
    });

    redraw_all();

    while (corriendo_ && cliente_.conectado()) {
        int ch = getch();
        if (ch == ERR) {
            // Sin input, solo redibujar si hace falta
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            redraw_all();
            continue;
        }

        if (ch == '\t') {
            // Cambiar de sala con Tab
            std::string nueva_sala;
            {
                std::lock_guard<std::mutex> lk(mutex_);
                if (!salas_.empty()) {
                    sala_idx_ = (sala_idx_ + 1) % static_cast<int>(salas_.size());
                    nueva_sala = salas_[static_cast<size_t>(sala_idx_)];
                }
            }
            if (!nueva_sala.empty()) cambiar_sala(nueva_sala);
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (!input_buffer_.empty()) {
                // Borrar último carácter UTF-8
                auto& s = input_buffer_;
                while (!s.empty() && (s.back() & 0xC0) == 0x80)
                    s.pop_back();
                if (!s.empty()) s.pop_back();
            }
        } else if (ch == '\n' || ch == KEY_ENTER) {
            procesar_input();
        } else if (ch >= 32 && ch < 256) {
            input_buffer_ += static_cast<char>(ch);
        } else if (ch == 3) { // Ctrl+C
            corriendo_ = false;
        }

        redraw_all();
    }

    cliente_.detener();
}

// ── Render ───────────────────────────────────────────────────────────────────

void TUI::redraw_all() {
    std::lock_guard<std::mutex> lk(mutex_);
    draw_status();
    draw_salas();
    draw_usuarios();
    draw_chat();
    draw_input();
    doupdate();
}

void TUI::draw_status() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    (void)rows;

    werase(win_status_);
    wbkgd(win_status_, COLOR_PAIR(7));
    wattron(win_status_, A_BOLD | COLOR_PAIR(7));
    std::string status = " Chat Server  |  Sala: #" + sala_activa_
                       + "  |  Usuario: " + cliente_.usuario()
                       + "  |  Tab: cambiar sala  |  /help: ayuda";
    mvwprintw(win_status_, 0, 0, "%-*s", cols - 1, status.c_str());
    wattroff(win_status_, A_BOLD | COLOR_PAIR(7));
    wnoutrefresh(win_status_);
}

void TUI::draw_salas() {
    int rows, cols;
    getmaxyx(win_salas_, rows, cols);

    werase(win_salas_);
    wattron(win_salas_, COLOR_PAIR(6) | A_DIM);
    box(win_salas_, 0, 0);
    wattroff(win_salas_, COLOR_PAIR(6) | A_DIM);
    wattron(win_salas_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_salas_, 0, 2, " Salas ");
    wattroff(win_salas_, COLOR_PAIR(2) | A_BOLD);

    int y = 1;
    for (int i = 0; i < static_cast<int>(salas_.size()) && y < rows - 1; ++i, ++y) {
        if (i == sala_idx_) {
            wattron(win_salas_, COLOR_PAIR(5) | A_BOLD);
            mvwprintw(win_salas_, y, 1, "%-*s", cols - 2,
                      ("#" + salas_[static_cast<size_t>(i)]).c_str());
            wattroff(win_salas_, COLOR_PAIR(5) | A_BOLD);
        } else {
            mvwprintw(win_salas_, y, 1, "#%-*s", cols - 3,
                      salas_[static_cast<size_t>(i)].c_str());
        }
    }
    wnoutrefresh(win_salas_);
}

void TUI::draw_usuarios() {
    int rows, cols;
    getmaxyx(win_usuarios_, rows, cols);

    werase(win_usuarios_);
    wattron(win_usuarios_, COLOR_PAIR(6) | A_DIM);
    box(win_usuarios_, 0, 0);
    wattroff(win_usuarios_, COLOR_PAIR(6) | A_DIM);
    wattron(win_usuarios_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_usuarios_, 0, 2, " Usuarios ");
    wattroff(win_usuarios_, COLOR_PAIR(2) | A_BOLD);

    int y = 1;
    for (auto& u : usuarios_) {
        if (y >= rows - 1) break;
        if (u == cliente_.usuario()) {
            wattron(win_usuarios_, COLOR_PAIR(3) | A_BOLD);
            mvwprintw(win_usuarios_, y++, 1, "%-*s", cols - 2, ("* " + u).c_str());
            wattroff(win_usuarios_, COLOR_PAIR(3) | A_BOLD);
        } else {
            mvwprintw(win_usuarios_, y++, 1, "%-*s", cols - 2, u.c_str());
        }
    }
    wnoutrefresh(win_usuarios_);
}

void TUI::draw_chat() {
    int rows, cols;
    getmaxyx(win_chat_, rows, cols);

    werase(win_chat_);
    wattron(win_chat_, COLOR_PAIR(6) | A_DIM);
    box(win_chat_, 0, 0);
    wattroff(win_chat_, COLOR_PAIR(6) | A_DIM);
    wattron(win_chat_, COLOR_PAIR(3) | A_BOLD);
    std::string titulo = " #" + sala_activa_ + " ";
    mvwprintw(win_chat_, 0, 2, "%s", titulo.c_str());
    wattroff(win_chat_, COLOR_PAIR(3) | A_BOLD);

    int area = rows - 2; // filas útiles dentro del borde
    int start = static_cast<int>(mensajes_.size()) > area
              ? static_cast<int>(mensajes_.size()) - area : 0;

    int y = 1;
    for (int i = start; i < static_cast<int>(mensajes_.size()) && y < rows - 1; ++i, ++y) {
        const auto& m = mensajes_[static_cast<size_t>(i)];
        wattron(win_chat_, COLOR_PAIR(m.color_pair));
        // Truncar si el texto supera el ancho
        std::string txt = m.texto;
        if (static_cast<int>(txt.size()) > cols - 3)
            txt = txt.substr(0, static_cast<size_t>(cols - 6)) + "...";
        mvwprintw(win_chat_, y, 1, "%s", txt.c_str());
        wattroff(win_chat_, COLOR_PAIR(m.color_pair));
    }
    wnoutrefresh(win_chat_);
}

void TUI::draw_input() {
    int rows, cols;
    getmaxyx(win_input_, rows, cols);
    (void)rows;

    werase(win_input_);
    wattron(win_input_, COLOR_PAIR(2) | A_DIM);
    mvwhline(win_input_, 0, 0, ACS_HLINE, cols);
    wattroff(win_input_, COLOR_PAIR(2) | A_DIM);

    // Prompt ">" en amarillo, texto de input en blanco
    wattron(win_input_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_input_, 1, 0, "> ");
    wattroff(win_input_, COLOR_PAIR(2) | A_BOLD);
    wattron(win_input_, COLOR_PAIR(1));
    wprintw(win_input_, "%-*s", cols - 3, input_buffer_.c_str());
    wattroff(win_input_, COLOR_PAIR(1));
    // Posicionar cursor al final del input
    wmove(win_input_, 1, static_cast<int>(2 + input_buffer_.size()));
    wnoutrefresh(win_input_);
}

// ── Callback del servidor ────────────────────────────────────────────────────

void TUI::on_mensaje(const json& msg) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (!msg.contains("tipo")) return;
    std::string tipo = msg["tipo"].get<std::string>();

    if (tipo == T_BROADCAST) {
        std::string de   = msg.value("usuario", "?");
        std::string cont = msg.value("contenido", "");
        bool es_propio   = (de == cliente_.usuario());
        agregar_mensaje("[" + de + "] " + cont, es_propio ? 3 : 1);

    } else if (tipo == T_SISTEMA) {
        agregar_mensaje("* " + msg.value("contenido", ""), 2);

    } else if (tipo == T_ERROR) {
        agregar_mensaje("[Error] " + msg.value("mensaje", ""), 4);

    } else if (tipo == T_BIENVENIDA) {
        agregar_mensaje(">> " + msg.value("contenido", ""), 2);

    } else if (tipo == T_LISTA_SALAS) {
        salas_.clear();
        if (msg.contains("salas")) {
            for (auto& s : msg["salas"]) salas_.push_back(s.get<std::string>());
        }
        // Actualizar índice
        auto it = std::find(salas_.begin(), salas_.end(), sala_activa_);
        sala_idx_ = (it != salas_.end())
                  ? static_cast<int>(std::distance(salas_.begin(), it)) : 0;

    } else if (tipo == T_LISTA_USERS) {
        usuarios_.clear();
        if (msg.contains("usuarios")) {
            for (auto& u : msg["usuarios"]) usuarios_.push_back(u.get<std::string>());
        }

    } else if (tipo == T_HISTORIAL) {
        if (msg.contains("mensajes")) {
            agregar_mensaje("── Historial ──", 2);
            for (auto& m : msg["mensajes"]) {
                std::string de   = m.value("usuario", "Sistema");
                std::string cont = m.value("contenido", "");
                agregar_mensaje("[" + de + "] " + cont, 1);
            }
            agregar_mensaje("── Fin del historial ──", 2);
        }

    } else if (tipo == "desconectado") {
        agregar_mensaje("[Sistema] Conexión con el servidor perdida.", 4);
        corriendo_ = false;
    }
}

void TUI::agregar_mensaje(const std::string& texto, int color) {
    mensajes_.push_back({texto, color});
    if (mensajes_.size() > MAX_MSGS) mensajes_.pop_front();
}

// ── Procesamiento de input ───────────────────────────────────────────────────

void TUI::procesar_input() {
    if (input_buffer_.empty()) return;

    std::string cmd = input_buffer_;
    input_buffer_.clear();

    if (cmd == "/help") {
        mostrar_ayuda();
    } else if (cmd.rfind("/sala ", 0) == 0) {
        cambiar_sala(cmd.substr(6));
    } else if (cmd.rfind("/nueva ", 0) == 0) {
        cliente_.enviar_crear_sala(cmd.substr(7));
    } else if (cmd.rfind("/usuarios", 0) == 0) {
        cliente_.enviar_listar_usuarios();
    } else if (cmd.rfind("/salas", 0) == 0) {
        cliente_.enviar_listar_salas();
    } else if (cmd == "/salir" || cmd == "/quit") {
        cliente_.enviar_salir();
        corriendo_ = false;
    } else if (cmd[0] == '/') {
        std::lock_guard<std::mutex> lk(mutex_);
        agregar_mensaje("[Error] Comando desconocido. Escribe /help", 4);
    } else {
        // Mensaje normal
        if (sala_activa_.empty()) {
            std::lock_guard<std::mutex> lk(mutex_);
            agregar_mensaje("[Error] Únete a una sala primero con /sala <nombre>", 4);
        } else {
            cliente_.enviar_mensaje(cmd);
        }
    }
}

void TUI::mostrar_ayuda() {
    std::lock_guard<std::mutex> lk(mutex_);
    agregar_mensaje("── Comandos disponibles ──────────────────", 2);
    agregar_mensaje("/sala <nombre>   - Unirse a una sala", 2);
    agregar_mensaje("/nueva <nombre>  - Crear nueva sala", 2);
    agregar_mensaje("/salas           - Listar salas", 2);
    agregar_mensaje("/usuarios        - Listar usuarios de la sala", 2);
    agregar_mensaje("/salir           - Salir del chat", 2);
    agregar_mensaje("Tab              - Cambiar de sala", 2);
    agregar_mensaje("──────────────────────────────────────────", 2);
}

void TUI::cambiar_sala(const std::string& sala) {
    sala_activa_ = sala;
    cliente_.set_sala_actual(sala);
    cliente_.enviar_unirse(sala);
    cliente_.enviar_listar_usuarios(sala);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        mensajes_.clear();
        agregar_mensaje("── Entrando a #" + sala + " ──", 2);
    }
}
