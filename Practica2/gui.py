import queue
import threading
import tkinter as tk
import time
import wave
from pathlib import Path
from tkinter import messagebox, ttk

from cliente import MusicClient

try:
    import winsound
except ImportError:
    winsound = None


class MusicPlayerGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Cliente UDP de canciones")
        self.geometry("1180x760")
        self.minsize(980, 640)
        self.configure(bg="#f4f0f7")

        self.client = MusicClient()
        self.songs = []
        self.downloaded_file = None
        self.playing = False
        self.paused = False
        self.play_token = 0
        self.finish_timer = None
        self.playback_offset = 0.0
        self.playback_started_at = None
        self.playback_temp_file = None
        self.events = queue.Queue()

        self.setup_styles()
        self.build_ui()
        self.after(100, self.process_events)
        self.load_songs()

    def setup_styles(self):
        self.purple = "#7f4a8b"
        self.text_color = "#2f2b32"
        self.muted = "#6f6873"
        self.border = "#d6cfda"
        self.panel_bg = "#fbf9fc"

        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("Root.TFrame", background="#f4f0f7")
        style.configure("Panel.TLabelframe", background="#f4f0f7", bordercolor=self.border)
        style.configure("Panel.TLabelframe.Label", background="#f4f0f7", foreground=self.text_color, font=("Segoe UI", 11, "bold"))
        style.configure("Card.TFrame", background=self.panel_bg, bordercolor=self.border, relief="solid")
        style.configure("CardTitle.TLabel", background=self.panel_bg, foreground=self.muted, font=("Segoe UI", 9))
        style.configure("CardValue.TLabel", background=self.panel_bg, foreground=self.purple, font=("Segoe UI", 10, "bold"))
        style.configure("TLabel", background="#f4f0f7", foreground=self.text_color)
        style.configure("Title.TLabel", background="#f4f0f7", foreground=self.text_color, font=("Segoe UI", 22, "bold"))
        style.configure("Control.TButton", foreground=self.purple, font=("Segoe UI", 10, "bold"), padding=(12, 10))
        style.map("Control.TButton", foreground=[("disabled", "#858085")])
        style.configure("Player.TButton", foreground=self.purple, font=("Segoe UI", 14, "bold"), padding=(18, 12))
        style.map("Player.TButton", foreground=[("disabled", "#858085")])
        style.configure("Horizontal.TProgressbar", troughcolor="#ece7ee", background=self.purple, bordercolor=self.border)

    def build_ui(self):
        root = ttk.Frame(self, style="Root.TFrame", padding=10)
        root.pack(fill="both", expand=True)

        title = ttk.Label(root, text="Cliente UDP de canciones", style="Title.TLabel")
        title.pack(pady=(6, 16))

        top = ttk.Frame(root, style="Root.TFrame")
        top.pack(fill="both", expand=False)
        top.columnconfigure(0, weight=1)
        top.columnconfigure(1, weight=0)

        songs_frame = ttk.LabelFrame(top, text="Canciones disponibles", style="Panel.TLabelframe", padding=10)
        songs_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
        songs_frame.rowconfigure(0, weight=1)
        songs_frame.columnconfigure(0, weight=1)

        self.listbox = tk.Listbox(
            songs_frame,
            height=11,
            activestyle="none",
            borderwidth=0,
            highlightthickness=0,
            selectbackground="#e9dced",
            selectforeground=self.text_color,
            font=("Segoe UI", 11),
            bg="white",
            fg=self.text_color,
        )
        self.listbox.grid(row=0, column=0, sticky="nsew")
        self.listbox.bind("<<ListboxSelect>>", self.on_select)

        scrollbar = ttk.Scrollbar(songs_frame, orient="vertical", command=self.listbox.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.listbox.configure(yscrollcommand=scrollbar.set)

        controls = ttk.LabelFrame(top, text="Controles", style="Panel.TLabelframe", padding=(8, 12))
        controls.grid(row=0, column=1, sticky="ns")

        self.refresh_button = ttk.Button(controls, text="Cargar lista", style="Control.TButton", command=self.load_songs)
        self.download_button = ttk.Button(controls, text="Descargar", style="Control.TButton", command=self.download_selected, state="disabled")
        self.play_button = ttk.Button(controls, text="Reproducir", style="Control.TButton", command=self.play_selected, state="disabled")
        self.pause_button = ttk.Button(controls, text="Pausar", style="Control.TButton", command=self.pause_playback, state="disabled")
        self.continue_button = ttk.Button(controls, text="Continuar", style="Control.TButton", command=self.continue_playback, state="disabled")
        self.stop_button = ttk.Button(controls, text="Detener", style="Control.TButton", command=lambda: self.stop_playback("Reproduccion detenida."), state="disabled")

        for button in (
            self.refresh_button,
            self.download_button,
            self.play_button,
            self.pause_button,
            self.continue_button,
            self.stop_button,
        ):
            button.pack(fill="x", pady=(0, 10), ipadx=22)

        log_frame = ttk.LabelFrame(root, text="Log de transferencia", style="Panel.TLabelframe", padding=10)
        log_frame.pack(fill="both", expand=True, pady=(10, 8))

        self.log_text = tk.Text(
            log_frame,
            height=10,
            wrap="word",
            borderwidth=0,
            highlightthickness=0,
            bg="white",
            fg="#1f1d22",
            font=("Consolas", 10),
        )
        self.log_text.pack(fill="both", expand=True)

        info = ttk.Frame(root, style="Root.TFrame")
        info.pack(fill="x", pady=(0, 8))
        info.columnconfigure(0, weight=1)
        info.columnconfigure(1, weight=1)
        info.columnconfigure(2, weight=1)

        self.selected_value = self.create_info_card(info, 0, "Cancion seleccionada", "Sin seleccion")
        self.downloaded_value = self.create_info_card(info, 1, "Archivo descargado", "Ninguna")
        self.player_value = self.create_info_card(info, 2, "Estado del reproductor", "Sin reproduccion activa")

        player = ttk.LabelFrame(root, text="Panel del reproductor", style="Panel.TLabelframe", padding=10)
        player.pack(fill="x")
        player.columnconfigure(1, weight=1)

        self.progress_label = ttk.Label(player, text="Sin descarga activa", background="#f4f0f7", font=("Segoe UI", 9, "bold"))
        self.progress_label.grid(row=0, column=1, sticky="ew", padx=(8, 0))

        self.progress = ttk.Progressbar(player, maximum=100, style="Horizontal.TProgressbar")
        self.progress.grid(row=1, column=1, sticky="ew", padx=(8, 0), pady=(0, 10))

        player_buttons = ttk.Frame(player, style="Root.TFrame")
        player_buttons.grid(row=2, column=0, columnspan=2, sticky="w")

        ttk.Button(player_buttons, text=">", style="Player.TButton", command=self.play_selected).pack(side="left", padx=(0, 8))
        ttk.Button(player_buttons, text="||", style="Player.TButton", command=self.pause_playback).pack(side="left", padx=(0, 8))
        ttk.Button(player_buttons, text="|>", style="Player.TButton", command=self.continue_playback).pack(side="left", padx=(0, 8))
        ttk.Button(player_buttons, text="[]", style="Player.TButton", command=lambda: self.stop_playback("Reproduccion detenida.")).pack(side="left", padx=(0, 14))
        ttk.Label(player_buttons, text="Usa los controles para reproducir, pausar o detener.", background="#f4f0f7", foreground=self.muted).pack(side="left")

    def create_info_card(self, parent, column, title, value):
        card = ttk.Frame(parent, style="Card.TFrame", padding=10)
        card.grid(row=0, column=column, sticky="ew", padx=(0 if column == 0 else 8, 0))
        ttk.Label(card, text=title, style="CardTitle.TLabel").pack(anchor="w")
        label = ttk.Label(card, text=value, style="CardValue.TLabel")
        label.pack(anchor="w", pady=(6, 0))
        return label

    def log(self, message):
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")

    def load_songs(self):
        self.set_busy(True)
        self.status_text("Pidiendo lista al servidor UDP...")
        self.listbox.delete(0, tk.END)
        self.listbox.insert(tk.END, "Cargando...")
        self.log("Solicitud de lista enviada al servidor.")

        def worker():
            try:
                songs = self.client.list_songs()
                self.events.put(("songs", songs))
            except Exception as error:
                self.events.put(("error", f"No se pudo pedir la lista: {error}"))

        threading.Thread(target=worker, daemon=True).start()

    def on_select(self, _event=None):
        selected = self.current_selection()
        if selected:
            self.selected_value.config(text=selected["name"])
            self.download_button.config(state="normal")
            self.log(f"Cancion seleccionada: {selected['name']}")
        else:
            self.selected_value.config(text="Sin seleccion")
            self.download_button.config(state="disabled")

    def current_selection(self):
        selection = self.listbox.curselection()
        if not selection or selection[0] >= len(self.songs):
            return None
        return self.songs[selection[0]]

    def download_selected(self):
        song = self.current_selection()
        if not song:
            self.status_text("Selecciona una cancion primero.")
            return

        filename = song["name"]
        self.set_busy(True)
        self.reset_player_state()
        self.progress["value"] = 0
        self.progress_label.config(text="0%")
        self.status_text(f"Descargando con sliding window: {filename}")
        self.log(f"Solicitud de descarga enviada: {filename}")

        def worker():
            try:
                def progress(received, total):
                    percent = int(received * 100 / total) if total else 0
                    self.events.put(("progress", (percent, received, total)))

                path = self.client.download_song(filename, progress)
                self.events.put(("downloaded", path))
            except Exception as error:
                self.events.put(("error", f"No se pudo descargar: {error}"))

        threading.Thread(target=worker, daemon=True).start()

    def play_selected(self):
        if not self.downloaded_file:
            self.status_text("Primero descarga una cancion completa.")
            return

        if winsound is None:
            messagebox.showinfo("Reproduccion", "La reproduccion integrada usa winsound y solo esta disponible en Windows.")
            return

        if self.playing:
            return

        token = self.start_playback_from_offset(self.playback_offset)
        self.playing = True
        self.paused = False
        self.play_button.config(state="disabled")
        self.pause_button.config(state="normal")
        self.continue_button.config(state="disabled")
        self.stop_button.config(state="normal")
        self.status_text(f"Reproduciendo: {self.downloaded_file.name}")
        self.schedule_finish_check(token)

    def pause_playback(self):
        if not self.playing:
            return

        self.playback_offset += time.monotonic() - self.playback_started_at
        winsound.PlaySound(None, 0)
        self.playing = False
        self.paused = True
        self.play_token += 1
        self.cancel_finish_timer()
        self.play_button.config(state="normal")
        self.pause_button.config(state="disabled")
        self.continue_button.config(state="normal")
        self.stop_button.config(state="normal")
        self.status_text(f"Pausado en {self.format_time(self.playback_offset)}.")

    def continue_playback(self):
        if not self.downloaded_file:
            return

        self.play_selected()

    def stop_playback(self, status_text):
        if winsound is not None:
            winsound.PlaySound(None, 0)

        self.playing = False
        self.paused = False
        self.play_token += 1
        self.playback_offset = 0.0
        self.playback_started_at = None
        self.cancel_finish_timer()
        self.remove_playback_temp()
        self.play_button.config(state="normal" if self.downloaded_file else "disabled")
        self.pause_button.config(state="disabled")
        self.continue_button.config(state="disabled")
        self.stop_button.config(state="disabled")
        self.status_text(status_text)

    def reset_player_state(self):
        if self.playing or self.paused:
            self.stop_playback("Reproduccion detenida.")
        self.downloaded_file = None
        self.downloaded_value.config(text="Ninguna")
        self.play_button.config(state="disabled")
        self.pause_button.config(state="disabled")
        self.continue_button.config(state="disabled")
        self.stop_button.config(state="disabled")

    def status_text(self, text):
        self.player_value.config(text=text)

    def set_busy(self, busy):
        state = "disabled" if busy else "normal"
        self.refresh_button.config(state=state)
        self.download_button.config(state=state if self.current_selection() else "disabled")

    def schedule_finish_check(self, token):
        duration_ms = self.audio_duration_ms(self.current_playback_file())
        self.finish_timer = self.after(duration_ms, lambda: self.finish_playback(token))

    def finish_playback(self, token):
        if token != self.play_token or not self.playing:
            return

        self.finish_timer = None
        self.playing = False
        self.paused = False
        self.playback_offset = 0.0
        self.playback_started_at = None
        self.remove_playback_temp()
        self.play_button.config(state="normal")
        self.pause_button.config(state="disabled")
        self.continue_button.config(state="disabled")
        self.stop_button.config(state="disabled")
        self.status_text("Reproduccion finalizada.")

    def cancel_finish_timer(self):
        if self.finish_timer is not None:
            self.after_cancel(self.finish_timer)
            self.finish_timer = None

    def audio_duration_ms(self, path):
        try:
            with wave.open(str(path), "rb") as audio:
                frames = audio.getnframes()
                rate = audio.getframerate()
                return max(100, int((frames / rate) * 1000))
        except Exception:
            return 1000

    def start_playback_from_offset(self, offset_seconds):
        self.play_token += 1
        self.remove_playback_temp()

        playback_file = self.downloaded_file
        if offset_seconds > 0:
            playback_file = self.create_playback_segment(offset_seconds)

        self.playback_started_at = time.monotonic()
        winsound.PlaySound(str(playback_file), winsound.SND_FILENAME | winsound.SND_ASYNC)
        return self.play_token

    def create_playback_segment(self, offset_seconds):
        temp_path = self.downloaded_file.parent / f"__reproduciendo_{self.downloaded_file.name}"

        with wave.open(str(self.downloaded_file), "rb") as source:
            params = source.getparams()
            start_frame = min(int(offset_seconds * source.getframerate()), source.getnframes())
            source.setpos(start_frame)
            frames = source.readframes(source.getnframes() - start_frame)

        with wave.open(str(temp_path), "wb") as target:
            target.setparams(params)
            target.writeframes(frames)

        self.playback_temp_file = temp_path
        return temp_path

    def current_playback_file(self):
        return self.playback_temp_file if self.playback_temp_file else self.downloaded_file

    def remove_playback_temp(self):
        if self.playback_temp_file and self.playback_temp_file.exists():
            try:
                self.playback_temp_file.unlink()
            except OSError:
                pass
        self.playback_temp_file = None

    def format_time(self, seconds):
        minutes = int(seconds // 60)
        remaining = int(seconds % 60)
        return f"{minutes}:{remaining:02d}"

    def process_events(self):
        try:
            while True:
                event, payload = self.events.get_nowait()

                if event == "songs":
                    self.songs = payload
                    self.listbox.delete(0, tk.END)
                    if not self.songs:
                        self.listbox.insert(tk.END, "El servidor no tiene canciones .wav")
                        self.log("Lista recibida. Canciones disponibles: 0")
                    else:
                        for song in self.songs:
                            mb = song["size"] / (1024 * 1024)
                            self.listbox.insert(tk.END, f"  - {song['name']}   {mb:.1f} MB")
                        self.log(f"Lista recibida. Canciones disponibles: {len(self.songs)}")
                    self.status_text("Sin reproduccion activa")
                    self.set_busy(False)

                elif event == "progress":
                    percent, received, total = payload
                    self.progress["value"] = percent
                    self.progress_label.config(text=f"{percent}%")
                    if percent == 100 or percent % 10 == 0:
                        self.log(f"Descarga: {percent}% ({received}/{total} bytes)")

                elif event == "downloaded":
                    self.downloaded_file = Path(payload)
                    self.progress["value"] = 100
                    self.progress_label.config(text="Descarga completa")
                    self.downloaded_value.config(text=self.downloaded_file.name)
                    self.play_button.config(state="normal")
                    self.stop_button.config(state="disabled")
                    self.status_text("Lista para reproducir")
                    self.log(f"Descarga completa: {self.downloaded_file.name}")
                    self.set_busy(False)

                elif event == "error":
                    self.set_busy(False)
                    self.status_text(payload)
                    self.log(payload)
                    messagebox.showerror("Error", payload)
        except queue.Empty:
            pass

        self.after(100, self.process_events)


if __name__ == "__main__":
    app = MusicPlayerGUI()
    app.mainloop()
