// ============================================================================
//  tui.cpp
//  ----------------------------------------------------------------------------
//  Implementacion de la interfaz de terminal con ncurses (ver tui.hpp).
// ============================================================================
#include "tui.hpp"
#include "base64.hpp"

#include <algorithm>
#include <sstream>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <filesystem>

// ── Constructor / Destructor ─────────────────────────────────────────────────

TUI::TUI(ChatClient& cliente) : cliente_(cliente) {
    init_ncurses();
    crear_ventanas();
}

TUI::~TUI() {
    limpiar();
}

// Inicializa ncurses y define los pares de color usados en toda la interfaz.
void TUI::init_ncurses() {
    setlocale(LC_ALL, "");   // habilita UTF-8 (acentos y emojis)
    initscr();               // arranca ncurses
    cbreak();                // las teclas llegan sin esperar Enter
    noecho();                // no eco automatico (lo dibujamos nosotros)
    keypad(stdscr, TRUE);    // habilita teclas especiales (flechas, etc.)
    nodelay(stdscr, TRUE);   // getch() no bloquea: devuelve ERR si no hay tecla
    curs_set(1);             // cursor visible

    if (has_colors()) {
        start_color();
        use_default_colors();  // permite usar -1 = color de fondo del terminal
        init_pair(1, COLOR_WHITE,   -1);              // texto normal
        init_pair(2, COLOR_YELLOW,  -1);              // sistema / info
        init_pair(3, COLOR_CYAN,    -1);              // mensaje propio
        init_pair(4, COLOR_RED,     -1);              // error
        init_pair(5, COLOR_MAGENTA, -1);              // sala seleccionada / flecha mencion
        init_pair(6, COLOR_WHITE,   -1);              // bordes / titulos
        init_pair(7, COLOR_BLACK,   COLOR_GREEN);     // barra de estado
        init_pair(8, COLOR_BLACK,   COLOR_YELLOW);    // resaltado @usuario
        init_pair(9, COLOR_GREEN,   -1);              // marcador de imagen
    }
}

// Calcula tamanos segun el terminal y crea las cinco ventanas (paneles).
void TUI::crear_ventanas() {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    int chat_w = cols - PANEL_W - 1;            // ancho del chat = todo menos el panel izq.
    int panel_h_salas    = rows / 2 - 1;        // mitad superior izquierda
    int panel_h_usuarios = rows - panel_h_salas - 3; // resto izquierdo

    win_status_   = newwin(1, cols, 0, 0);                       // fila 0
    win_salas_    = newwin(panel_h_salas,    PANEL_W, 1, 0);
    win_usuarios_ = newwin(panel_h_usuarios, PANEL_W, 1 + panel_h_salas, 0);
    win_chat_     = newwin(rows - 3, chat_w, 1, PANEL_W + 1);
    win_input_    = newwin(2, cols, rows - 2, 0);                // dos ultimas filas
}

// Libera las ventanas y devuelve la terminal a su estado normal.
void TUI::limpiar() {
    if (win_salas_)    { delwin(win_salas_);    win_salas_    = nullptr; }
    if (win_usuarios_) { delwin(win_usuarios_); win_usuarios_ = nullptr; }
    if (win_chat_)     { delwin(win_chat_);     win_chat_     = nullptr; }
    if (win_status_)   { delwin(win_status_);   win_status_   = nullptr; }
    if (win_input_)    { delwin(win_input_);    win_input_    = nullptr; }
    endwin();
}

// ── Bucle principal ──────────────────────────────────────────────────────────

// Registra el callback de recepcion y entra al bucle de teclado/redibujo.
// Como getch() es no bloqueante, cuando no hay tecla dormimos un poco y
// redibujamos (asi se ven los mensajes que llegan por el otro hilo).
void TUI::run() {
    cliente_.iniciar_recepcion([this](const json& msg) {
        on_mensaje(msg);
    });

    redraw_all();

    while (corriendo_ && cliente_.conectado()) {
        int ch = getch();
        if (ch == ERR) {                 // no se pulso nada
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            redraw_all();
            continue;
        }

        if (ch == '\t') {
            // Tab: pasar a la siguiente sala de la lista (ciclico).
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
            // Borrar un caracter; con cuidado por UTF-8 (un emoji son varios bytes).
            if (!input_buffer_.empty()) {
                auto& s = input_buffer_;
                while (!s.empty() && (s.back() & 0xC0) == 0x80) // bytes de continuacion
                    s.pop_back();
                if (!s.empty()) s.pop_back();                   // byte inicial
            }
        } else if (ch == '\n' || ch == KEY_ENTER) {
            procesar_input();                                   // enviar lo escrito
        } else if (ch >= 32 && ch < 256) {
            input_buffer_ += static_cast<char>(ch);             // caracter normal
        } else if (ch == 3) {                                   // Ctrl+C
            corriendo_ = false;
        }

        redraw_all();
    }

    cliente_.detener();
}

// ── Render ───────────────────────────────────────────────────────────────────

// Redibuja todos los paneles. Se toma el mutex porque lee estado que el hilo de
// recepcion puede estar modificando. doupdate() vuelca todo a pantalla de golpe.
void TUI::redraw_all() {
    std::lock_guard<std::mutex> lk(mutex_);
    draw_status();
    draw_salas();
    draw_usuarios();
    draw_chat();
    draw_input();
    doupdate();
}

// Barra superior: nombre de la sala, usuario y recordatorios de teclas.
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
    wnoutrefresh(win_status_);  // marca para refrescar; el volcado real es doupdate()
}

// Panel de salas. Resalta la sala seleccionada por Tab (sala_idx_).
void TUI::draw_salas() {
    int rows, cols;
    getmaxyx(win_salas_, rows, cols);

    werase(win_salas_);
    wattron(win_salas_, COLOR_PAIR(6) | A_DIM);
    box(win_salas_, 0, 0);                       // borde tenue
    wattroff(win_salas_, COLOR_PAIR(6) | A_DIM);
    wattron(win_salas_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_salas_, 0, 2, " Salas ");       // titulo en el borde
    wattroff(win_salas_, COLOR_PAIR(2) | A_BOLD);

    int y = 1;
    for (int i = 0; i < static_cast<int>(salas_.size()) && y < rows - 1; ++i, ++y) {
        if (i == sala_idx_) {                     // sala enfocada
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

// Panel de usuarios de la sala actual. Marca al usuario propio con "* " y color.
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

// Panel central de chat. Muestra las ultimas lineas que caben (auto-scroll):
// calcula desde donde empezar para que el ultimo mensaje quede abajo.
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

    int area = rows - 2; // filas utiles dentro del borde
    int start = static_cast<int>(mensajes_.size()) > area
              ? static_cast<int>(mensajes_.size()) - area : 0;

    int y = 1;
    for (int i = start; i < static_cast<int>(mensajes_.size()) && y < rows - 1; ++i, ++y) {
        draw_linea_chat(y, cols, mensajes_[static_cast<size_t>(i)]);
    }
    wnoutrefresh(win_chat_);
}

// Pinta UNA linea del chat resaltando los tokens @usuario y, si el mensaje me
// menciona, marcandolo en negrita con una flecha al inicio.
// Importante: se imprime por SEGMENTOS (no byte a byte) usando wprintw, que
// respeta UTF-8; rastreamos la posicion con getyx para no pasarnos del ancho.
void TUI::draw_linea_chat(int y, int cols, const MensajeTUI& m) {
    const std::string& txt = m.texto;

    int base_attr = COLOR_PAIR(m.color_pair);
    if (m.destacado) base_attr |= A_BOLD;

    // Prefijo de mencion: flecha "►".
    wmove(win_chat_, y, 1);
    if (m.destacado) {
        wattron(win_chat_, COLOR_PAIR(5) | A_BOLD);
        waddch(win_chat_, ACS_RARROW);
        waddch(win_chat_, ' ');
        wattroff(win_chat_, COLOR_PAIR(5) | A_BOLD);
    }

    // Imprime un trozo con cierto atributo. Devuelve false si ya no cabe mas.
    auto imprime_segmento = [&](const std::string& s, int attr) -> bool {
        int cy, cx;
        getyx(win_chat_, cy, cx);
        (void)cy;
        if (cx >= cols - 1) return false;
        wattron(win_chat_, attr);
        wprintw(win_chat_, "%s", s.c_str());
        wattroff(win_chat_, attr);
        getyx(win_chat_, cy, cx);
        return cx < cols - 1;
    };

    // Recorre el texto: cuando encuentra un @palabra, imprime el texto previo con
    // el color base y la mencion con el color de resaltado (par 8).
    size_t i = 0, ini = 0;
    while (i < txt.size()) {
        if (txt[i] == '@') {
            size_t j = i + 1;
            while (j < txt.size() &&
                   (std::isalnum(static_cast<unsigned char>(txt[j])) || txt[j] == '_'))
                ++j;
            if (j > i + 1) { // hay al menos un caracter despues de @
                if (i > ini && !imprime_segmento(txt.substr(ini, i - ini), base_attr)) return;
                if (!imprime_segmento(txt.substr(i, j - i), COLOR_PAIR(8) | A_BOLD)) return;
                i = ini = j;
                continue;
            }
        }
        ++i;
    }
    if (ini < txt.size())
        imprime_segmento(txt.substr(ini), base_attr);
}

// Linea de entrada inferior: separador + prompt ">" + lo que se va escribiendo.
void TUI::draw_input() {
    int rows, cols;
    getmaxyx(win_input_, rows, cols);
    (void)rows;

    werase(win_input_);
    wattron(win_input_, COLOR_PAIR(2) | A_DIM);
    mvwhline(win_input_, 0, 0, ACS_HLINE, cols);  // linea separadora
    wattroff(win_input_, COLOR_PAIR(2) | A_DIM);

    wattron(win_input_, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win_input_, 1, 0, "> ");
    wattroff(win_input_, COLOR_PAIR(2) | A_BOLD);
    wattron(win_input_, COLOR_PAIR(1));
    wprintw(win_input_, "%-*s", cols - 3, input_buffer_.c_str());
    wattroff(win_input_, COLOR_PAIR(1));
    // Dejar el cursor justo al final del texto escrito.
    wmove(win_input_, 1, static_cast<int>(2 + input_buffer_.size()));
    wnoutrefresh(win_input_);
}

// ── Callback del servidor (se ejecuta en el hilo de recepcion) ───────────────

// Convierte cada mensaje del servidor en cambios de estado de la TUI. Toma el
// mutex porque corre en otro hilo distinto al del render/teclado.
void TUI::on_mensaje(const json& msg) {
    std::lock_guard<std::mutex> lk(mutex_);

    if (!msg.contains("tipo")) return;
    std::string tipo = msg["tipo"].get<std::string>();

    if (tipo == T_BROADCAST) {
        // Mensaje de alguien: color segun si es mio o de otro.
        std::string de   = msg.value("usuario", "?");
        std::string cont = msg.value("contenido", "");
        bool es_propio   = (de == cliente_.usuario());
        agregar_mensaje("[" + de + "] " + cont, es_propio ? 3 : 1);
        // Si me mencionan (y no soy yo), marcar la linea como destacada.
        if (!es_propio && me_mencionan(cont))
            mensajes_.back().destacado = true;

    } else if (tipo == T_IMAGEN) {
        recibir_imagen(msg);

    } else if (tipo == T_SISTEMA) {
        agregar_mensaje("* " + msg.value("contenido", ""), 2);

    } else if (tipo == T_ERROR) {
        agregar_mensaje("[Error] " + msg.value("mensaje", ""), 4);

    } else if (tipo == T_BIENVENIDA) {
        agregar_mensaje(">> " + msg.value("contenido", ""), 2);

    } else if (tipo == T_LISTA_SALAS) {
        // Reemplazar la lista de salas y recolocar el indice en la sala activa.
        salas_.clear();
        if (msg.contains("salas")) {
            for (auto& s : msg["salas"]) salas_.push_back(s.get<std::string>());
        }
        auto it = std::find(salas_.begin(), salas_.end(), sala_activa_);
        sala_idx_ = (it != salas_.end())
                  ? static_cast<int>(std::distance(salas_.begin(), it)) : 0;

    } else if (tipo == T_LISTA_USERS) {
        usuarios_.clear();
        if (msg.contains("usuarios")) {
            for (auto& u : msg["usuarios"]) usuarios_.push_back(u.get<std::string>());
        }

    } else if (tipo == T_HISTORIAL) {
        // Volcar los mensajes previos de la sala entre dos separadores.
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
        // Mensaje sintetico del ChatClient cuando se cae la conexion.
        agregar_mensaje("[Sistema] Conexión con el servidor perdida.", 4);
        corriendo_ = false;
    }
}

// Agrega una linea simple al chat, respetando el tope MAX_MSGS (cola acotada).
// OJO: asume que el llamador ya tiene el mutex tomado.
void TUI::agregar_mensaje(const std::string& texto, int color) {
    mensajes_.push_back({texto, color});
    if (mensajes_.size() > MAX_MSGS) mensajes_.pop_front();
}

// ── Procesamiento de la entrada del usuario ──────────────────────────────────

// Interpreta lo que el usuario escribio al pulsar Enter: si empieza por '/' es
// un comando; si no, es un mensaje normal para la sala actual.
void TUI::procesar_input() {
    if (input_buffer_.empty()) return;

    std::string cmd = input_buffer_;
    input_buffer_.clear();

    if (cmd == "/help") {
        mostrar_ayuda();
    } else if (cmd.rfind("/sala ", 0) == 0) {        // rfind(...,0)==0 => "empieza por"
        cambiar_sala(cmd.substr(6));
    } else if (cmd.rfind("/nueva ", 0) == 0) {
        cliente_.enviar_crear_sala(cmd.substr(7));
    } else if (cmd.rfind("/usuarios", 0) == 0) {
        cliente_.enviar_listar_usuarios();
    } else if (cmd.rfind("/salas", 0) == 0) {
        cliente_.enviar_listar_salas();
    } else if (cmd.rfind("/img ", 0) == 0) {
        // Enviar imagen y reportar exito/error en el chat.
        std::string ruta = cmd.substr(5);
        std::string err = cliente_.enviar_imagen(ruta);
        std::lock_guard<std::mutex> lk(mutex_);
        if (err.empty())
            agregar_mensaje("* Enviando imagen: " + ruta, 2);
        else
            agregar_mensaje("[Error] " + err, 4);
    } else if (cmd.rfind("/ver", 0) == 0) {
        // /ver [n]; sin numero => la ultima (n = -1).
        int n = -1;
        if (cmd.size() > 5) { try { n = std::stoi(cmd.substr(5)); } catch (...) {} }
        ver_imagen(n);
    } else if (cmd == "/salir" || cmd == "/quit") {
        cliente_.enviar_salir();
        corriendo_ = false;
    } else if (cmd[0] == '/') {
        std::lock_guard<std::mutex> lk(mutex_);
        agregar_mensaje("[Error] Comando desconocido. Escribe /help", 4);
    } else {
        // Texto normal: requiere estar en una sala.
        if (sala_activa_.empty()) {
            std::lock_guard<std::mutex> lk(mutex_);
            agregar_mensaje("[Error] Únete a una sala primero con /sala <nombre>", 4);
        } else {
            cliente_.enviar_mensaje(cmd);
        }
    }
}

// Imprime la lista de comandos en el propio chat (toma el mutex).
void TUI::mostrar_ayuda() {
    std::lock_guard<std::mutex> lk(mutex_);
    agregar_mensaje("── Comandos disponibles ──────────────────", 2);
    agregar_mensaje("/sala <nombre>   - Unirse a una sala", 2);
    agregar_mensaje("/nueva <nombre>  - Crear nueva sala", 2);
    agregar_mensaje("/salas           - Listar salas", 2);
    agregar_mensaje("/usuarios        - Listar usuarios de la sala", 2);
    agregar_mensaje("/img <ruta>      - Enviar una imagen a la sala", 2);
    agregar_mensaje("/ver [n]         - Ver imagen recibida (n, o la ultima)", 2);
    agregar_mensaje("@usuario         - Mencionar a alguien (se resalta)", 2);
    agregar_mensaje("/salir           - Salir del chat", 2);
    agregar_mensaje("Tab              - Cambiar de sala", 2);
    agregar_mensaje("──────────────────────────────────────────", 2);
}

// Cambia la sala activa: avisa al servidor (unirse + pedir usuarios) y limpia el
// area de chat para mostrar solo la conversacion de la nueva sala.
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

// ── Menciones ────────────────────────────────────────────────────────────────

// ¿El texto contiene "@miusuario" como palabra completa? Sin distinguir
// mayusculas/minusculas. El "limite de palabra" evita que "@Ana" dispare con
// "@Anabel": tras el nombre no debe venir letra/digito/_.
bool TUI::me_mencionan(const std::string& texto) const {
    std::string objetivo = "@" + cliente_.usuario();
    auto baja = [](std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    };
    std::string t = baja(texto);
    std::string o = baja(objetivo);
    size_t pos = 0;
    while ((pos = t.find(o, pos)) != std::string::npos) {
        size_t fin = pos + o.size();
        if (fin >= t.size() ||
            !(std::isalnum(static_cast<unsigned char>(t[fin])) || t[fin] == '_'))
            return true;
        pos = fin;
    }
    return false;
}

// ── Imagenes ─────────────────────────────────────────────────────────────────

// Procesa una imagen recibida: la decodifica de base64 a un archivo temporal en
// /tmp/chat_p5/, la registra en imagenes_ y agrega un marcador clicable (/ver N)
// al chat. Asume el mutex tomado (lo llama on_mensaje).
void TUI::recibir_imagen(const json& msg) {
    namespace fs = std::filesystem;
    std::string de      = msg.value("usuario", "?");
    std::string nombre  = msg.value("nombre_archivo", "imagen");
    std::string datos   = msg.value("datos", "");

    if (datos.empty()) {
        agregar_mensaje("[Error] Imagen vacia de " + de, 4);
        return;
    }

    // Carpeta de cache de la sesion.
    fs::path dir = fs::temp_directory_path() / "chat_p5";
    std::error_code ec;
    fs::create_directories(dir, ec);

    int idx = static_cast<int>(imagenes_.size());
    // Quitar cualquier ruta del nombre por seguridad; quedarnos solo el archivo.
    std::string seguro = fs::path(nombre).filename().string();
    if (seguro.empty()) seguro = "imagen";
    fs::path destino = dir / (std::to_string(idx) + "_" + seguro);

    // Decodificar y escribir a disco.
    std::vector<uint8_t> bytes = b64::decode(datos);
    std::ofstream out(destino, std::ios::binary);
    if (!out) {
        agregar_mensaje("[Error] No se pudo guardar la imagen de " + de, 4);
        return;
    }
    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    out.close();

    imagenes_.push_back({seguro, destino.string(), de});

    // Marcador visible con el indice para usar luego "/ver N".
    MensajeTUI m;
    m.texto = "🖼  [" + std::to_string(idx) + "] " + seguro
            + " (de " + de + ")  —  /ver " + std::to_string(idx);
    m.color_pair = 9;
    m.img_idx = idx;
    mensajes_.push_back(m);
    if (mensajes_.size() > MAX_MSGS) mensajes_.pop_front();
}

// Muestra una imagen ya recibida. Suspende temporalmente ncurses para devolver
// el control de la terminal y la pinta con 'kitty +kitten icat' (Kitty Graphics
// Protocol) si esta disponible; si no, la abre con el visor del sistema.
//   n: indice de la imagen; -1 = la ultima recibida.
void TUI::ver_imagen(int n) {
    int idx;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (imagenes_.empty()) {
            agregar_mensaje("[Error] No has recibido ninguna imagen aun", 4);
            return;
        }
        idx = (n < 0) ? static_cast<int>(imagenes_.size()) - 1 : n;
        if (idx < 0 || idx >= static_cast<int>(imagenes_.size())) {
            agregar_mensaje("[Error] Indice de imagen invalido: " + std::to_string(n), 4);
            return;
        }
    }
    std::string ruta = imagenes_[static_cast<size_t>(idx)].ruta_local;
    std::string nom  = imagenes_[static_cast<size_t>(idx)].nombre;

    // Guardar el modo de ncurses y soltar la terminal (modo cocido normal).
    def_prog_mode();
    endwin();

    std::printf("\n=== Imagen [%d] %s ===\n\n", idx, nom.c_str());
    std::fflush(stdout);

    // ¿Hay binario 'kitty'? (presente si usas la terminal Kitty).
    bool hay_kitty = (std::system("command -v kitty >/dev/null 2>&1") == 0);
    bool ok = false;
    if (hay_kitty) {
        std::string cmd = "kitty +kitten icat --align left \"" + ruta + "\"";
        ok = (std::system(cmd.c_str()) == 0);
    }
    if (!ok) {
        // Fallback universal: abrir con el visor de imagenes del sistema.
        std::printf("(Kitty no disponible — abriendo con el visor del sistema)\n");
        std::fflush(stdout);
        std::string cmd = "xdg-open \"" + ruta + "\" >/dev/null 2>&1 &";
        std::system(cmd.c_str());
    }

    // Esperar Enter para no perder la imagen de vista.
    std::printf("\n\nPresiona ENTER para volver al chat...");
    std::fflush(stdout);
    int c;
    while ((c = std::getchar()) != '\n' && c != EOF) {}

    // Restaurar ncurses y redibujar la interfaz.
    reset_prog_mode();
    refresh();
    redraw_all();
}
