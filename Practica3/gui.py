import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import messagebox, ttk

from cliente import MusicClient, PipeMp3Player


class MusicPlayerGUI(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Practica 3 - UDP, tuberia e hilos")
        self.geometry("1180x760")
        self.minsize(1020, 660)

        self.client = MusicClient()
        self.player = PipeMp3Player()
        self.events = queue.Queue()
        self.songs = []
        self.current_result = None
        self.transfer_thread = None
        self.playback_thread = None
        self.audio_active = False
        self.audio_paused = False

        self.setup_styles()
        self.build_ui()
        self.after(100, self.process_events)
        self.load_songs()

    def setup_styles(self):
        self.bg = "#f4f0f7"
        self.panel = "#ffffff"
        self.text = "#2f2b32"
        self.muted = "#6f6873"
        self.blue = "#6f3f86"
        self.green = "#178566"
        self.orange = "#bf6b19"

        self.configure(bg=self.bg)
        style = ttk.Style(self)
        style.theme_use("clam")
        style.configure("TFrame", background=self.bg)
        style.configure("Panel.TLabelframe", background=self.bg, bordercolor="#d6cfda")
        style.configure("Panel.TLabelframe.Label", background=self.bg, foreground=self.text, font=("Segoe UI", 11, "bold"))
        style.configure("TLabel", background=self.bg, foreground=self.text)
        style.configure("Title.TLabel", background=self.bg, foreground=self.text, font=("Segoe UI", 22, "bold"))
        style.configure("Subtitle.TLabel", background=self.bg, foreground=self.muted, font=("Segoe UI", 10))
        style.configure("Action.TButton", foreground=self.blue, font=("Segoe UI", 10, "bold"), padding=(12, 9))
        style.configure("Hero.TLabel", background=self.bg, foreground=self.blue, font=("Segoe UI", 10, "bold"))
        style.configure("Horizontal.TProgressbar", troughcolor="#ece7ee", background=self.blue)

    def build_ui(self):
        root = ttk.Frame(self, padding=12)
        root.pack(fill="both", expand=True)

        ttk.Label(root, text="Practica 3: streaming MP3 con UDP, hilos y tuberia", style="Title.TLabel").pack(anchor="w")
        ttk.Label(
            root,
            text="Reproductor interno: no abre Windows Media Player. La transferencia UDP y la reproduccion viven en hilos separados unidos por una tuberia.",
            style="Subtitle.TLabel",
        ).pack(anchor="w", pady=(0, 12))
        ttk.Label(root, text="UDP + ACK acumulado | Pipe con buffer grande | Metadatos ID3 | Reproduccion interna MCI", style="Hero.TLabel").pack(anchor="w", pady=(0, 12))

        main = ttk.Frame(root)
        main.pack(fill="both", expand=True)
        main.columnconfigure(0, weight=2)
        main.columnconfigure(1, weight=1)
        main.rowconfigure(0, weight=1)

        songs_frame = ttk.LabelFrame(main, text="Canciones MP3 disponibles", style="Panel.TLabelframe", padding=10)
        songs_frame.grid(row=0, column=0, sticky="nsew", padx=(0, 10))
        songs_frame.rowconfigure(0, weight=1)
        songs_frame.columnconfigure(0, weight=1)

        self.listbox = tk.Listbox(
            songs_frame,
            borderwidth=0,
            highlightthickness=1,
            highlightbackground="#ccd3df",
            selectbackground="#dce7ff",
            font=("Segoe UI", 11),
            bg="white",
            fg=self.text,
        )
        self.listbox.grid(row=0, column=0, sticky="nsew")
        self.listbox.bind("<<ListboxSelect>>", self.on_select)
        scrollbar = ttk.Scrollbar(songs_frame, orient="vertical", command=self.listbox.yview)
        scrollbar.grid(row=0, column=1, sticky="ns")
        self.listbox.configure(yscrollcommand=scrollbar.set)

        side = ttk.LabelFrame(main, text="Control de practica 3", style="Panel.TLabelframe", padding=10)
        side.grid(row=0, column=1, sticky="nsew")

        self.refresh_button = ttk.Button(side, text="Cargar lista", style="Action.TButton", command=self.load_songs)
        self.stream_button = ttk.Button(side, text="Iniciar streaming UDP", style="Action.TButton", command=self.stream_selected, state="disabled")
        self.pause_button = ttk.Button(side, text="Pausar audio", style="Action.TButton", command=self.pause_audio, state="disabled")
        self.resume_button = ttk.Button(side, text="Reanudar audio", style="Action.TButton", command=self.resume_audio, state="disabled")
        self.stop_button = ttk.Button(side, text="Detener audio interno", style="Action.TButton", command=self.stop_audio, state="disabled")
        self.refresh_button.pack(fill="x", pady=(0, 8))
        self.stream_button.pack(fill="x", pady=(0, 8))
        self.pause_button.pack(fill="x", pady=(0, 8))
        self.resume_button.pack(fill="x", pady=(0, 8))
        self.stop_button.pack(fill="x", pady=(0, 18))

        self.meta_title = self.info(side, "Titulo", "Sin seleccion")
        self.meta_artist = self.info(side, "Artista", "-")
        self.meta_album = self.info(side, "Album", "-")
        self.meta_year = self.info(side, "Anio", "-")
        self.meta_genre = self.info(side, "Genero", "-")

        bottom = ttk.Frame(root)
        bottom.pack(fill="x", pady=(12, 0))
        bottom.columnconfigure(0, weight=1)
        bottom.columnconfigure(1, weight=1)
        bottom.columnconfigure(2, weight=1)

        self.transfer_state = self.status_card(bottom, 0, "Hilo de transferencia", "Esperando")
        self.playback_state = self.status_card(bottom, 1, "Hilo de reproduccion interna", "Esperando")
        self.pipe_state = self.status_card(bottom, 2, "Tuberia / buffer", "0 chunks")

        progress_frame = ttk.LabelFrame(root, text="Progreso UDP con ventana deslizante", style="Panel.TLabelframe", padding=10)
        progress_frame.pack(fill="x", pady=(12, 0))
        self.progress_label = ttk.Label(progress_frame, text="Sin transferencia activa")
        self.progress_label.pack(anchor="w")
        self.progress = ttk.Progressbar(progress_frame, maximum=100, style="Horizontal.TProgressbar")
        self.progress.pack(fill="x", pady=(6, 0))

        log_frame = ttk.LabelFrame(root, text="Log", style="Panel.TLabelframe", padding=10)
        log_frame.pack(fill="both", expand=True, pady=(12, 0))
        self.log_text = tk.Text(log_frame, height=8, bg="white", fg="#1e2430", font=("Consolas", 10), borderwidth=0)
        self.log_text.pack(fill="both", expand=True)

    def info(self, parent, label, value):
        ttk.Label(parent, text=label, foreground=self.muted).pack(anchor="w")
        item = ttk.Label(parent, text=value, font=("Segoe UI", 10, "bold"), wraplength=300)
        item.pack(anchor="w", pady=(0, 8))
        return item

    def status_card(self, parent, column, title, value):
        frame = ttk.Frame(parent, padding=10)
        frame.grid(row=0, column=column, sticky="ew", padx=(0 if column == 0 else 8, 0))
        ttk.Label(frame, text=title, foreground=self.muted).pack(anchor="w")
        label = ttk.Label(frame, text=value, font=("Segoe UI", 10, "bold"))
        label.pack(anchor="w", pady=(5, 0))
        return label

    def log(self, message):
        self.log_text.insert("end", message + "\n")
        self.log_text.see("end")

    def set_busy(self, busy):
        self.refresh_button.config(state="disabled" if busy else "normal")
        self.stream_button.config(state="disabled" if busy or not self.current_selection() else "normal")
        self.update_audio_buttons()

    def update_audio_buttons(self):
        self.pause_button.config(state="normal" if self.audio_active and not self.audio_paused else "disabled")
        self.resume_button.config(state="normal" if self.audio_active and self.audio_paused else "disabled")
        self.stop_button.config(state="normal" if self.audio_active else "disabled")

    def load_songs(self):
        self.set_busy(True)
        self.listbox.delete(0, tk.END)
        self.listbox.insert(tk.END, "Cargando lista MP3...")
        self.log("LIST enviado por socket UDP.")

        def worker():
            try:
                self.events.put(("songs", self.client.list_songs()))
            except Exception as error:
                self.events.put(("error", f"No se pudo cargar la lista: {error}"))

        threading.Thread(target=worker, daemon=True).start()

    def current_selection(self):
        selection = self.listbox.curselection()
        if not selection or selection[0] >= len(self.songs):
            return None
        return self.songs[selection[0]]

    def on_select(self, _event=None):
        song = self.current_selection()
        if not song:
            return
        self.meta_title.config(text=song.get("title", song["name"]))
        self.meta_artist.config(text=song.get("artist", "Desconocido"))
        self.meta_album.config(text=song.get("album", "Desconocido"))
        self.meta_year.config(text=song.get("year", "N/D"))
        self.meta_genre.config(text=song.get("genre", "N/D"))
        self.stream_button.config(state="normal")
        self.log(f"Seleccion: {song['name']} | {song.get('artist')} - {song.get('album')}")

    def stream_selected(self):
        song = self.current_selection()
        if not song:
            return

        self.progress["value"] = 0
        self.progress_label.config(text="0%")
        self.transfer_state.config(text="Recibiendo paquetes UDP")
        self.playback_state.config(text="Esperando datos en tuberia")
        self.pipe_state.config(text="0 chunks / 0 bytes")
        self.audio_active = False
        self.audio_paused = False
        self.update_audio_buttons()
        self.set_busy(True)
        self.log("Iniciando dos hilos: transferencia y reproduccion interna.")
        self.log("La tuberia comunica el flujo ordenado recibido hacia el reproductor MCI embebido.")

        def progress(received, total):
            self.events.put(("progress", (received, total)))

        def pipe_status(chunks, bytes_buffered):
            self.events.put(("pipe", (chunks, bytes_buffered)))

        def playback_status(status, bytes_available):
            self.events.put(("playback", (status, bytes_available)))

        self.current_result, self.transfer_thread, self.playback_thread, _pipe = self.client.start_streaming(
            song["name"],
            self.player,
            progress=progress,
            pipe_status=pipe_status,
            playback_status=playback_status,
        )
        self.after(300, self.watch_stream)

    def pause_audio(self):
        ok, message = self.player.pause()
        if ok:
            self.audio_paused = True
            self.playback_state.config(text="Audio interno pausado")
            self.log("Audio interno pausado.")
        else:
            self.log(f"No se pudo pausar: {message}")
        self.update_audio_buttons()

    def resume_audio(self):
        ok, message = self.player.resume()
        if ok:
            self.audio_paused = False
            self.playback_state.config(text="Audio interno reanudado")
            self.log("Audio interno reanudado.")
        else:
            self.log(f"No se pudo reanudar: {message}")
        self.update_audio_buttons()

    def stop_audio(self):
        self.player.stop_audio()
        self.audio_active = False
        self.audio_paused = False
        self.playback_state.config(text="Audio interno detenido")
        self.log("Audio interno detenido.")
        self.update_audio_buttons()

    def stop_stream(self):
        self.player.stop()
        self.audio_active = False
        self.audio_paused = False
        self.log("Solicitud de detener reproduccion y tuberia enviada.")
        self.update_audio_buttons()

    def watch_stream(self):
        if not self.current_result:
            return

        if self.transfer_thread.is_alive() or self.playback_thread.is_alive():
            self.after(300, self.watch_stream)
            return

        result = self.current_result
        self.current_result = None
        self.set_busy(False)
        if result.error:
            self.transfer_state.config(text="Error")
            self.playback_state.config(text="Detenida")
            self.audio_active = False
            self.audio_paused = False
            self.update_audio_buttons()
            messagebox.showerror("Error", str(result.error))
            self.log(f"Error: {result.error}")
        else:
            self.transfer_state.config(text="Descarga completa")
            if not self.audio_active:
                self.playback_state.config(text="Tuberia consumida")
            self.progress["value"] = 100
            self.progress_label.config(text=f"Archivo guardado: {Path(result.path).name}")
            self.log(f"Descarga finalizada: {result.path}")

    def process_events(self):
        try:
            while True:
                event, payload = self.events.get_nowait()
                if event == "songs":
                    self.songs = payload
                    self.listbox.delete(0, tk.END)
                    if not self.songs:
                        self.listbox.insert(tk.END, "Coloca archivos .mp3 en canciones/")
                        self.log("Lista recibida: 0 canciones MP3.")
                    else:
                        for song in self.songs:
                            mb = song["size"] / (1024 * 1024)
                            self.listbox.insert(tk.END, f"{song.get('title')} - {song.get('artist')} ({mb:.1f} MB)")
                        self.log(f"Lista recibida: {len(self.songs)} canciones MP3 con metadatos.")
                    self.set_busy(False)

                elif event == "progress":
                    received, total = payload
                    percent = int(received * 100 / total) if total else 0
                    self.progress["value"] = percent
                    self.progress_label.config(text=f"{percent}% ({received}/{total} bytes)")
                    self.transfer_state.config(text=f"UDP + ACK acumulado: {percent}%")

                elif event == "pipe":
                    chunks, bytes_buffered = payload
                    self.pipe_state.config(text=f"{chunks} chunks / {bytes_buffered} bytes")

                elif event == "playback":
                    status, bytes_available = payload
                    self.playback_state.config(text=f"{status}: {bytes_available} bytes")
                    status_lower = status.lower()
                    if "reproduciendo" in status_lower or "activo" in status_lower:
                        self.audio_active = True
                        self.audio_paused = False
                        self.update_audio_buttons()

                elif event == "error":
                    self.set_busy(False)
                    self.log(payload)
                    messagebox.showerror("Error", payload)
        except queue.Empty:
            pass

        self.after(100, self.process_events)


if __name__ == "__main__":
    app = MusicPlayerGUI()
    app.mainloop()
