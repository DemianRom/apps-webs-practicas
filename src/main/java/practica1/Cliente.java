package practica1;

/*
 * =====================================================================
 *  APLICACIONES PARA COMUNICACIONES EN RED — ESCOM
 *  Unidad Temática I: Sockets de flujo
 *  Práctica 1: Servicio de transferencia de archivos
 * =====================================================================
 *
 *  ARCHIVO: Cliente.java
 *
 *  ALUMNOS:
 *      - Demian Romero Bautista  (lógica de red, protocolo)
 *      - Said Ferreira
 *      - Mateo Alejandro Jaimes Uribe       (GUI — ver sección [GUI] abajo)
 *
 * =====================================================================
 *
 *  ╔═══════════════════════════════════════════════════════════════╗
 *  ║               ARQUITECTURA DE RED — DOS CANALES               ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║                                                               ║
 *  ║  CANAL DE METADATOS  puerto 8000  PERMANENTE                  ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  · Se abre al iniciar sesión, se cierra al salir.             ║
 *  ║  · Transporta JSON de UNA SOLA LÍNEA por mensaje.             ║
 *  ║  · Delimitador de mensaje: el salto de línea \n               ║
 *  ║    (PrintWriter.println / BufferedReader.readLine).           ║
 *  ║  · Protocolo estricto: el cliente envía, el servidor          ║
 *  ║    responde. Nunca dos mensajes sin respuesta entre medio.    ║
 *  ║                                                               ║
 *  ║  CANAL DE DATOS      puerto 8001  INTERMITENTE                ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  · Se abre justo antes de transferir bytes y se cierra        ║
 *  ║    al terminar. El servidor hace accept() cada vez.           ║
 *  ║  · Transporta bytes crudos (InputStream / OutputStream).      ║
 *  ║  · Soporta cualquier tipo de archivo sin corrupción.          ║
 *  ║  · Se leen EXACTAMENTE los bytes indicados en "tamanio":      ║
 *  ║    TCP es un flujo continuo y no sabe dónde termina un        ║
 *  ║    archivo; leer hasta EOF bloquea el socket para siempre.    ║
 *  ║                                                               ║
 *  ╚═══════════════════════════════════════════════════════════════╝
 *
 * =====================================================================
 *
 *  ╔═══════════════════════════════════════════════════════════════╗
 *  ║         CONTRATO JSON — PROTOCOLO COMPLETO                    ║
 *  ║                                                               ║
 *  ║  Todos los mensajes son objetos JSON en UNA sola línea.       ║
 *  ║  El cliente SIEMPRE envía primero; el servidor responde.      ║
 *  ║  Cada readLine() corresponde a exactamente UN mensaje.        ║
 *  ║                                                               ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║  COMANDOS  (cliente → servidor)                               ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║                                                               ║
 *  ║  Listar archivos remotos:                                     ║
 *  ║    {"cmd":"LIST"}                                             ║
 *  ║                                                               ║
 *  ║  Subir un archivo:                                            ║
 *  ║    {"cmd":"UPLOAD","nombre":"foto.jpg","tamanio":204800}      ║
 *  ║                                                               ║
 *  ║  Descargar un archivo:                                        ║
 *  ║    {"cmd":"DOWNLOAD","nombre":"reporte.pdf"}                  ║
 *  ║                                                               ║
 *  ║  Confirmar descarga completada:                               ║
 *  ║    {"cmd":"DOWNLOAD_OK"}                                      ║
 *  ║                                                               ║
 *  ║  Subir una carpeta (avisar cuántos archivos vienen):          ║
 *  ║    {"cmd":"UPLOAD_DIR","nombre":"fotos","cantidad":3}         ║
 *  ║                                                               ║
 *  ║  Enviar un archivo dentro de una carpeta (sub-protocolo):     ║
 *  ║    {"cmd":"FILE","nombre":"fotos/img.jpg","tamanio":81920}    ║
 *  ║    El campo "nombre" incluye la carpeta padre para que        ║
 *  ║    el servidor sepa en qué subdirectorio guardar el archivo.  ║
 *  ║                                                               ║
 *  ║  Descargar una carpeta:                                       ║
 *  ║    {"cmd":"DOWNLOAD_DIR","nombre":"musica"}                   ║
 *  ║                                                               ║
 *  ║  Confirmar recepción de cada archivo de carpeta:              ║
 *  ║    {"cmd":"NEXT_OK"}                                          ║
 *  ║                                                               ║
 *  ║  Borrar archivo remoto:                                       ║
 *  ║    {"cmd":"DELETE","nombre":"viejo.txt"}                      ║
 *  ║                                                               ║
 *  ║  Renombrar archivo remoto:                                    ║
 *  ║    {"cmd":"RENAME_FILE","actual":"a.txt","nuevo":"b.txt"}     ║
 *  ║                                                               ║
 *  ║  Renombrar carpeta remota:                                    ║
 *  ║    {"cmd":"RENAME_DIR","actual":"dir_a","nuevo":"dir_b"}      ║
 *  ║                                                               ║
 *  ║  Cerrar sesión:                                               ║
 *  ║    {"cmd":"EXIT"}                                             ║
 *  ║                                                               ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║  RESPUESTAS  (servidor → cliente)                             ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║                                                               ║
 *  ║  Éxito genérico (DELETE_OK, RENAME_OK, FILE_OK, UPLOAD_OK):  ║
 *  ║    {"status":"OK"}                                            ║
 *  ║    {"status":"DELETE_OK"}                                     ║
 *  ║    {"status":"RENAME_OK"}                                     ║
 *  ║    {"status":"FILE_OK"}                                       ║
 *  ║    {"status":"UPLOAD_OK"}                                     ║
 *  ║                                                               ║
 *  ║  Éxito con tamaño (respuesta a DOWNLOAD):                     ║
 *  ║    {"status":"OK","tamanio":512000}                           ║
 *  ║                                                               ║
 *  ║  Éxito con cantidad (respuesta a DOWNLOAD_DIR):               ║
 *  ║    {"status":"OK","cantidad":4}                               ║
 *  ║                                                               ║
 *  ║  Ítem de lista (se repite hasta FIN_LIST):                    ║
 *  ║    {"status":"ITEM","tipo":"FILE","nombre":"x","tamanio":100} ║
 *  ║    {"status":"ITEM","tipo":"DIR","nombre":"fotos","tamanio":0}║
 *  ║    {"status":"FIN_LIST"}                                      ║
 *  ║                                                               ║
 *  ║  Siguiente archivo al descargar carpeta:                      ║
 *  ║    {"status":"NEXT","nombre":"cancion.mp3","tamanio":4200000} ║
 *  ║                                                               ║
 *  ║  Error en cualquier operación:                                ║
 *  ║    {"status":"ERROR","msg":"Archivo no encontrado"}           ║
 *  ║                                                               ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║  REGLA DE ORO                                                 ║
 *  ║  El cliente llama a leerRespuesta() UNA vez por cada comando  ║
 *  ║  enviado. La única excepción es LIST, que lee en bucle hasta  ║
 *  ║  recibir {"status":"FIN_LIST"}.                               ║
 *  ╚═══════════════════════════════════════════════════════════════╝
 *
 * =====================================================================
 *
 *  ╔═══════════════════════════════════════════════════════════════╗
 *  ║         GUÍA PARA EL IMPLEMENTADOR DE LA GUI  (Mateo)         ║
 *  ╠═══════════════════════════════════════════════════════════════╣
 *  ║                                                               ║
 *  ║  ESTRUCTURA SUGERIDA DE LA VENTANA PRINCIPAL                  ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  · Panel izquierdo  → lista de archivos LOCALES  (JTable)     ║
 *  ║  · Panel derecho    → lista de archivos REMOTOS  (JTable)     ║
 *  ║  · Barra de botones → una acción por botón (ver lista abajo)  ║
 *  ║  · Barra de estado  → texto de la última operación            ║
 *  ║  · Barra de progreso (JProgressBar) → visible solo durante    ║
 *  ║    transferencias, oculta el resto del tiempo                 ║
 *  ║                                                               ║
 *  ║  REGLA IMPORTANTE — NUNCA BLOQUEAR EL EDT                     ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  Todas las llamadas de red (subirArchivo, descargarArchivo,   ║
 *  ║  etc.) son bloqueantes. Si las llamas desde un listener de    ║
 *  ║  botón, la ventana se congela hasta que terminen.             ║
 *  ║  Solución: envolver cada operación de red en un SwingWorker:  ║
 *  ║                                                               ║
 *  ║    new SwingWorker<Void, Integer>() {                         ║
 *  ║        protected Void doInBackground() {                      ║
 *  ║            cliente.subirArchivo(nombre);  // llamada de red   ║
 *  ║            return null;                                       ║
 *  ║        }                                                      ║
 *  ║        protected void done() {                                ║
 *  ║            refrescarListaLocal();  // actualizar la UI        ║
 *  ║        }                                                      ║
 *  ║    }.execute();                                               ║
 *  ║                                                               ║
 *  ║  BARRA DE PROGRESO                                            ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  En los bucles de transferencia hay comentarios marcados:     ║
 *  ║    // [GUI-PROGRESO]                                          ║
 *  ║  En esos puntos calcula el porcentaje y llama:                ║
 *  ║    publish( (int)(totalEnviado * 100 / tamanio) );            ║
 *  ║  Luego en process(List<Integer> chunks) actualiza:            ║
 *  ║    progressBar.setValue(chunks.get(chunks.size()-1));         ║
 *  ║                                                               ║
 *  ║  ACCIONES Y SU COMPONENTE GUI SUGERIDO                        ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  Listar local      → se hace automático al abrir/refrescar    ║
 *  ║  Listar remoto     → botón "Actualizar" en panel derecho      ║
 *  ║  Subir archivo     → botón "↑ Subir" + JFileChooser           ║
 *  ║  Descargar archivo → botón "↓ Bajar" sobre ítem seleccionado  ║
 *  ║  Subir carpeta     → JFileChooser en modo DIRECTORIES_ONLY    ║
 *  ║  Descargar carpeta → igual que descargar archivo              ║
 *  ║  Borrar local      → tecla Supr sobre ítem + JOptionPane      ║
 *  ║  Borrar remoto     → tecla Supr sobre ítem + JOptionPane      ║
 *  ║  Renombrar local   → doble clic sobre ítem en JTable          ║
 *  ║  Renombrar remoto  → doble clic sobre ítem en JTable          ║
 *  ║  Renombrar carpeta → igual que renombrar archivo              ║
 *  ║  Salir             → windowClosing() del JFrame               ║
 *  ║                                                               ║
 *  ║  CONFIRMACIONES OBLIGATORIAS                                  ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  Antes de borrar (local o remoto) mostrar siempre:            ║
 *  ║    JOptionPane.showConfirmDialog(                             ║
 *  ║        frame,                                                 ║
 *  ║        "¿Eliminar '" + nombre + "'?",                         ║
 *  ║        "Confirmar eliminación",                               ║
 *  ║        JOptionPane.YES_NO_OPTION);                            ║
 *  ║                                                               ║
 *  ║  INICIO DE SESIÓN                                             ║
 *  ║  ─────────────────────────────────────────────────────────    ║
 *  ║  Mostrar un JDialog al arrancar con campos:                   ║
 *  ║    · IP del servidor  (default "127.0.0.1")                   ║
 *  ║    · Puerto           (default 8000)                          ║
 *  ║    · Carpeta local    (JTextField + botón "Explorar")         ║
 *  ║  Esos valores se pasan a setServerIp(), setPuertoMeta() y     ║
 *  ║  setCarpetaLocal() antes de llamar a conectarMetadatos().     ║
 *  ║                                                               ║
 *  ║  BUSCA EN EL CÓDIGO:  // [GUI]                                ║
 *  ║  Cada línea marcada con ese comentario indica exactamente      ║
 *  ║  qué valor debe provenir de la interfaz gráfica.              ║
 *  ╚═══════════════════════════════════════════════════════════════╝
 *
 * =====================================================================
 */

import java.io.*;
import java.net.*;
import java.util.*;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;

public class Cliente {

    // -----------------------------------------------------------------
    //  CONFIGURACIÓN DE RED
    //  [GUI] Setear con setServerIp(), setPuertoMeta() y
    //        setCarpetaLocal() desde el diálogo de inicio de sesión,
    //        ANTES de llamar a conectarMetadatos().
    // -----------------------------------------------------------------
    private static String serverIp    = "127.0.0.1";
    private static int    puertoMeta  = 8000;
    private static int    puertoDatos = 8001;

    private static final int BUFFER = 4096; // bloque de lectura/escritura (4 KB)

    // -----------------------------------------------------------------
    //  CANAL DE METADATOS  (permanente durante la sesión)
    // -----------------------------------------------------------------
    private static Socket        socketMeta;
    private static PrintWriter   salida;   // envía JSON al servidor
    private static BufferedReader entrada; // recibe JSON del servidor

    // -----------------------------------------------------------------
    //  CARPETA LOCAL
    //  [GUI] Cambiar con setCarpetaLocal() antes de conectar.
    // -----------------------------------------------------------------
    private static String carpetaLocal =
            System.getProperty("user.home") + File.separator + "cliente_archivos";


    // =================================================================
    //  SETTERS DE CONFIGURACIÓN
    //  [GUI] Llamar desde el diálogo de inicio de sesión.
    // =================================================================

    /** Establece la IP del servidor antes de conectar. */
    public static void setServerIp(String ip) {
        serverIp = ip;
    }

    /** Establece el puerto del canal de metadatos antes de conectar. */
    public static void setPuertoMeta(int puerto) {
        puertoMeta = puerto;
    }

    /** Establece la carpeta local de trabajo. Crea el directorio si no existe. */
    public static void setCarpetaLocal(String ruta) {
        carpetaLocal = ruta;
        crearCarpetaLocalSiNoExiste();
    }

    /** Devuelve la ruta de la carpeta local actual. */
    public static String getCarpetaLocal() {
        return carpetaLocal;
    }


    // =================================================================
    //  CONEXIÓN Y DESCONEXIÓN
    // =================================================================

    /**
     * Abre el canal de METADATOS (puerto configurable, por defecto 8000).
     *
     * PrintWriter con autoFlush=true: cada println() envía los datos
     * de inmediato sin esperar a que el buffer se llene. Es obligatorio
     * en un protocolo pregunta-respuesta donde el servidor espera el
     * mensaje completo antes de responder.
     *
     * [GUI] Llamar desde el botón "Conectar" del diálogo de inicio de
     *       sesión, después de haber llamado a setServerIp(),
     *       setPuertoMeta() y setCarpetaLocal().
     *
     * @return true si la conexión fue exitosa.
     */
    public static boolean conectarMetadatos() {
        try {
            socketMeta = new Socket(serverIp, puertoMeta);

            salida  = new PrintWriter(
                    new BufferedWriter(
                            new OutputStreamWriter(socketMeta.getOutputStream())), true);

            entrada = new BufferedReader(
                    new InputStreamReader(socketMeta.getInputStream()));

            return true;

        } catch (IOException e) {
            // [GUI] Mostrar JOptionPane.showMessageDialog con e.getMessage()
            return false;
        }
    }

    /**
     * Cierra el canal de metadatos. Se llama una sola vez al terminar la sesión.
     *
     * [GUI] Llamar desde windowClosing() del JFrame principal.
     */
    public static void cerrarConexionMetadatos() {
        try {
            if (socketMeta != null && !socketMeta.isClosed()) socketMeta.close();
        } catch (IOException e) {
            // [GUI] Loguear si se desea, pero no es crítico al cerrar.
        }
    }

    /**
     * Abre un socket nuevo en el canal de DATOS (puerto puertoMeta + 1).
     *
     * Intermitente: existe solo durante la transferencia de bytes.
     * El servidor tiene un ServerSocket.accept() esperando cada vez
     * que se abre este canal.
     *
     * @return Socket abierto, o null si falló la conexión.
     */
    private static Socket abrirCanalDatos() {
        try {
            return new Socket(serverIp, puertoDatos);
        } catch (IOException e) {
            // [GUI] Mostrar error de conexión de datos si se desea
            return null;
        }
    }

    /**
     * Cierra el canal de datos al terminar cada transferencia.
     *
     * @param s Socket a cerrar.
     */
    private static void cerrarCanalDatos(Socket s) {
        try {
            if (s != null && !s.isClosed()) s.close();
        } catch (IOException ignored) {}
    }


    // =================================================================
    //  UTILIDADES JSON
    //  Único punto de construcción y parseo de mensajes.
    // =================================================================

    /**
     * Serializa el JsonObject y lo envía al servidor como una línea.
     * El \n al final de println() actúa como delimitador de mensaje.
     */
    private static void enviarJson(JsonObject json) {
        salida.println(json.toString());
    }

    /**
     * Lee una línea del canal de metadatos y la parsea como JSON.
     *
     * BLOQUEANTE — espera hasta que el servidor envíe una línea completa.
     * Llamar exactamente UNA vez por cada comando enviado
     * (excepto en LIST, que llama en bucle hasta FIN_LIST).
     *
     * @return JsonObject con la respuesta del servidor, null si cerró la conexión.
     * @throws IOException si hay error de red.
     */
    private static JsonObject leerRespuesta() throws IOException {
        String linea = entrada.readLine();
        if (linea == null) return null;
        return JsonParser.parseString(linea).getAsJsonObject();
    }

    /**
     * Devuelve true si la respuesta NO es un error.
     * El campo "status" con valor "ERROR" es el único caso de fallo.
     */
    private static boolean esExito(JsonObject resp) {
        if (resp == null) return false;
        return !resp.get("status").getAsString().equals("ERROR");
    }


    // =================================================================
    //  OPCIÓN 1 — LISTAR CARPETA LOCAL  (sin red)
    // =================================================================

    /**
     * Devuelve los archivos y subdirectorios de la carpeta local.
     * Operación 100 % local — no usa la red en ningún momento.
     *
     * [GUI] Llamar para poblar el JTable del panel izquierdo.
     *       Invocar automáticamente al arrancar y después de cualquier
     *       operación local (borrar, renombrar, descargar).
     *       Columnas sugeridas: Nombre | Tipo | Tamaño | Fecha de modificación.
     *
     * @return Array de File con el contenido de carpetaLocal,
     *         o array vacío si la carpeta está vacía o no existe.
     */
    public static File[] listarLocal() {
        File[] items = new File(carpetaLocal).listFiles();
        return items != null ? items : new File[0];
    }


    // =================================================================
    //  OPCIÓN 2 — LISTAR CARPETA REMOTA
    // =================================================================

    /**
     * Modelo de dato para un ítem de la lista remota.
     * [GUI] Usar para construir las filas del JTable del panel derecho.
     */
    public static class ItemRemoto {
        public final String tipo;    // "FILE" o "DIR"
        public final String nombre;
        public final long   tamanio;

        public ItemRemoto(String tipo, String nombre, long tamanio) {
            this.tipo    = tipo;
            this.nombre  = nombre;
            this.tamanio = tamanio;
        }
    }

    /**
     * Pide al servidor su lista de archivos y la devuelve como lista.
     *
     * Protocolo:
     *   C→S: {"cmd":"LIST"}
     *   S→C: {"status":"ITEM","tipo":"FILE","nombre":"x","tamanio":100}
     *        {"status":"ITEM","tipo":"DIR","nombre":"fotos","tamanio":0}
     *        ... (una línea por entrada)
     *        {"status":"FIN_LIST"}
     *
     * [GUI] Llamar para poblar el JTable del panel derecho.
     *       Ejecutar en SwingWorker si la carpeta remota puede ser grande.
     *
     * @return Lista de ItemRemoto, vacía si hay error o no hay archivos.
     */
    public static List<ItemRemoto> listarRemoto() {
        List<ItemRemoto> lista = new ArrayList<>();
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd", "LIST");
            enviarJson(cmd);

            JsonObject resp;
            while ((resp = leerRespuesta()) != null) {
                String status = resp.get("status").getAsString();
                if (status.equals("FIN_LIST")) break;
                if (status.equals("ITEM")) {
                    lista.add(new ItemRemoto(
                            resp.get("tipo").getAsString(),
                            resp.get("nombre").getAsString(),
                            resp.get("tamanio").getAsLong()
                    ));
                }
            }
        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error de red
        }
        return lista;
    }


    // =================================================================
    //  OPCIÓN 3 — SUBIR ARCHIVO
    // =================================================================

    /**
     * Envía un archivo local al servidor.
     *
     * Protocolo (3 fases):
     *   Fase 1 — negociación:
     *     C→S: {"cmd":"UPLOAD","nombre":"foto.jpg","tamanio":204800}
     *     S→C: {"status":"OK"}
     *          o {"status":"ERROR","msg":"Sin espacio en disco"}
     *
     *   Fase 2 — transferencia de bytes por canal de datos:
     *     [cliente abre puerto 8001 y escribe tamanio bytes]
     *
     *   Fase 3 — confirmación:
     *     S→C: {"status":"UPLOAD_OK"}
     *
     * [GUI] · nombre viene de un JFileChooser con currentDirectory = carpetaLocal.
     *       · Ejecutar en SwingWorker para no bloquear la interfaz.
     *       · Actualizar barra de progreso en los puntos [GUI-PROGRESO].
     *       · Al terminar (done()), refrescar la lista remota.
     *
     * @param nombre Nombre del archivo dentro de carpetaLocal a enviar.
     * @return true si el archivo fue subido correctamente.
     */
    public static boolean subirArchivo(String nombre) {
        File archivo = new File(carpetaLocal, nombre);

        if (!archivo.exists() || !archivo.isFile()) return false;

        long tamanio = archivo.length();

        try {
            // --- Fase 1: negociación ---
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",     "UPLOAD");
            cmd.addProperty("nombre",  nombre);
            cmd.addProperty("tamanio", tamanio);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) return false;

            // --- Fase 2: transferencia ---
            Socket canalDatos = abrirCanalDatos();
            if (canalDatos == null) return false;

            try (FileInputStream fis = new FileInputStream(archivo);
                 OutputStream    os  = canalDatos.getOutputStream()) {

                byte[] buf          = new byte[BUFFER];
                int    n;
                long   totalEnviado = 0;

                while ((n = fis.read(buf)) != -1) {
                    os.write(buf, 0, n);
                    totalEnviado += n;
                    // [GUI-PROGRESO] publish( (int)(totalEnviado * 100 / tamanio) );
                }
                os.flush();

            } finally {
                cerrarCanalDatos(canalDatos);
            }

            // --- Fase 3: confirmación ---
            JsonObject conf = leerRespuesta();
            return conf != null && conf.get("status").getAsString().equals("UPLOAD_OK");

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }


    // =================================================================
    //  OPCIÓN 4 — DESCARGAR ARCHIVO
    // =================================================================

    /**
     * Descarga un archivo remoto y lo guarda en la carpeta local.
     *
     * Protocolo (3 fases):
     *   Fase 1 — solicitud:
     *     C→S: {"cmd":"DOWNLOAD","nombre":"reporte.pdf"}
     *     S→C: {"status":"OK","tamanio":512000}
     *          o {"status":"ERROR","msg":"Archivo no encontrado"}
     *
     *   Fase 2 — recepción de bytes por canal de datos:
     *     [cliente abre puerto 8001 y lee EXACTAMENTE tamanio bytes]
     *
     *   Fase 3 — notificación al servidor:
     *     C→S: {"cmd":"DOWNLOAD_OK"}
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable remoto.
     *       · Ejecutar en SwingWorker.
     *       · Actualizar barra de progreso en [GUI-PROGRESO].
     *       · Al terminar (done()), refrescar la lista local.
     *
     * @param nombre Nombre del archivo remoto a descargar.
     * @return true si la descarga fue exitosa.
     */
    public static boolean descargarArchivo(String nombre) {
        try {
            // --- Fase 1: solicitar ---
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DOWNLOAD");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) return false;

            long tamanio = resp.get("tamanio").getAsLong();

            // --- Fase 2: recibir bytes ---
            Socket canalDatos = abrirCanalDatos();
            if (canalDatos == null) return false;

            File destino = new File(carpetaLocal, nombre);

            try (InputStream         is  = canalDatos.getInputStream();
                 FileOutputStream    fos = new FileOutputStream(destino);
                 BufferedOutputStream bos = new BufferedOutputStream(fos)) {

                byte[] buf     = new byte[BUFFER];
                long restantes = tamanio;

                while (restantes > 0) {
                    int aLeer = (int) Math.min(BUFFER, restantes);
                    int leido = is.read(buf, 0, aLeer);
                    if (leido == -1) break;
                    bos.write(buf, 0, leido);
                    restantes -= leido;
                    // [GUI-PROGRESO] publish( (int)((tamanio - restantes) * 100 / tamanio) );
                }
                bos.flush();

            } finally {
                cerrarCanalDatos(canalDatos);
            }

            // --- Fase 3: notificar ---
            JsonObject ok = new JsonObject();
            ok.addProperty("cmd", "DOWNLOAD_OK");
            enviarJson(ok);

            return true;

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }


    // =================================================================
    //  OPCIÓN 5 — SUBIR CARPETA
    // =================================================================

    /**
     * Envía todos los archivos directos de una carpeta local al servidor.
     * (No es recursivo; los subdirectorios se ignoran por ahora.)
     *
     * Protocolo:
     *   C→S: {"cmd":"UPLOAD_DIR","nombre":"fotos","cantidad":3}
     *   S→C: {"status":"OK"}
     *   [llama a enviarArchivoConCanal() por cada archivo]
     *
     * [GUI] · JFileChooser en modo DIRECTORIES_ONLY.
     *       · Ejecutar en SwingWorker.
     *       · Al terminar (done()), refrescar la lista remota.
     *
     * @param nombre Nombre de la subcarpeta dentro de carpetaLocal a enviar.
     * @return true si todos los archivos fueron enviados correctamente.
     */
    public static boolean subirCarpeta(String nombre) {
        File carpeta = new File(carpetaLocal, nombre);

        if (!carpeta.exists() || !carpeta.isDirectory()) return false;

        File[] todos = carpeta.listFiles();
        List<File> archivos = new ArrayList<>();
        if (todos != null)
            for (File f : todos)
                if (f.isFile()) archivos.add(f);

        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",      "UPLOAD_DIR");
            cmd.addProperty("nombre",   nombre);
            cmd.addProperty("cantidad", archivos.size());
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) return false;

            for (File archivo : archivos)
                enviarArchivoConCanal(nombre + "/" + archivo.getName(), archivo);

            return true;

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }


    // =================================================================
    //  OPCIÓN 6 — DESCARGAR CARPETA
    // =================================================================

    /**
     * Descarga una carpeta completa del servidor y la recrea localmente.
     *
     * Protocolo:
     *   C→S: {"cmd":"DOWNLOAD_DIR","nombre":"musica"}
     *   S→C: {"status":"OK","cantidad":4}
     *        o {"status":"ERROR","msg":"..."}
     *   [repite cantidad veces:]
     *     S→C: {"status":"NEXT","nombre":"cancion.mp3","tamanio":4200000}
     *     [cliente abre canal de datos y recibe tamanio bytes]
     *     C→S: {"cmd":"NEXT_OK"}
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable remoto.
     *       · Ejecutar en SwingWorker.
     *       · Al terminar (done()), refrescar la lista local.
     *
     * @param nombre Nombre de la carpeta remota a descargar.
     * @return true si la carpeta fue descargada correctamente.
     */
    public static boolean descargarCarpeta(String nombre) {
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DOWNLOAD_DIR");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) return false;

            int cantidad = resp.get("cantidad").getAsInt();

            File carpetaDest = new File(carpetaLocal, nombre);
            carpetaDest.mkdirs();

            for (int i = 0; i < cantidad; i++) {
                JsonObject meta       = leerRespuesta();
                String     nombreArch = meta.get("nombre").getAsString();
                long       tamanio    = meta.get("tamanio").getAsLong();

                Socket canalDatos = abrirCanalDatos();
                if (canalDatos == null) return false;

                recibirArchivoConCanal(canalDatos, new File(carpetaDest, nombreArch), tamanio);
                cerrarCanalDatos(canalDatos);

                JsonObject ack = new JsonObject();
                ack.addProperty("cmd", "NEXT_OK");
                enviarJson(ack);
            }

            return true;

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }


    // =================================================================
    //  OPERACIONES LOCALES  (sin red)  — opciones 7, 9, 11
    // =================================================================

    /**
     * Elimina un archivo de la carpeta local.
     * Operación 100 % local, no usa la red.
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable local.
     *       · Mostrar JOptionPane.showConfirmDialog antes de llamar.
     *       · Refrescar la lista local al terminar.
     *
     * @param nombre Nombre del archivo a eliminar dentro de carpetaLocal.
     * @return true si fue eliminado correctamente.
     */
    public static boolean borrarArchivoLocal(String nombre) {
        File archivo = new File(carpetaLocal, nombre);
        return archivo.exists() && archivo.isFile() && archivo.delete();
    }

    /**
     * Renombra un archivo en la carpeta local.
     * Operación 100 % local, no usa la red.
     *
     * [GUI] · Abrir un JOptionPane.showInputDialog con el nombre actual
     *         pre-cargado para que el usuario solo edite lo que cambia.
     *       · Refrescar la lista local al terminar.
     *
     * @param actual Nombre actual del archivo.
     * @param nuevo  Nuevo nombre.
     * @return true si fue renombrado correctamente.
     */
    public static boolean renombrarArchivoLocal(String actual, String nuevo) {
        File origen = new File(carpetaLocal, actual);
        File dest   = new File(carpetaLocal, nuevo);
        return origen.exists() && origen.isFile() && origen.renameTo(dest);
    }

    /**
     * Renombra una carpeta en la carpeta local.
     * Operación 100 % local, no usa la red.
     *
     * [GUI] · La fila seleccionada debe ser de tipo DIR.
     *       · Refrescar la lista local al terminar.
     *
     * @param actual Nombre actual de la carpeta.
     * @param nuevo  Nuevo nombre.
     * @return true si fue renombrada correctamente.
     */
    public static boolean renombrarCarpetaLocal(String actual, String nuevo) {
        File origen = new File(carpetaLocal, actual);
        File dest   = new File(carpetaLocal, nuevo);
        return origen.exists() && origen.isDirectory() && origen.renameTo(dest);
    }


    // =================================================================
    //  OPERACIONES REMOTAS SIMPLES  — opciones 8, 10, 12
    // =================================================================

    /**
     * Pide al servidor que elimine un archivo remoto.
     *
     * Protocolo:
     *   C→S: {"cmd":"DELETE","nombre":"viejo.txt"}
     *   S→C: {"status":"DELETE_OK"}
     *        o {"status":"ERROR","msg":"..."}
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable remoto.
     *       · Mostrar JOptionPane.showConfirmDialog antes de llamar.
     *       · Refrescar la lista remota al terminar.
     *
     * @param nombre Nombre del archivo remoto a eliminar.
     * @return true si fue eliminado correctamente.
     */
    public static boolean borrarArchivoRemoto(String nombre) {
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DELETE");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            return resp != null && resp.get("status").getAsString().equals("DELETE_OK");

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }

    /**
     * Pide al servidor que renombre un archivo remoto.
     *
     * Protocolo:
     *   C→S: {"cmd":"RENAME_FILE","actual":"a.txt","nuevo":"b.txt"}
     *   S→C: {"status":"RENAME_OK"}
     *        o {"status":"ERROR","msg":"..."}
     *
     * [GUI] · actual = fila seleccionada en el JTable remoto.
     *       · nuevo  = JOptionPane.showInputDialog con actual pre-cargado.
     *       · Refrescar la lista remota al terminar.
     *
     * @param actual Nombre actual del archivo remoto.
     * @param nuevo  Nuevo nombre.
     * @return true si fue renombrado correctamente.
     */
    public static boolean renombrarArchivoRemoto(String actual, String nuevo) {
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "RENAME_FILE");
            cmd.addProperty("actual", actual);
            cmd.addProperty("nuevo",  nuevo);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            return esExito(resp);

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }

    /**
     * Pide al servidor que renombre una carpeta remota.
     *
     * Protocolo:
     *   C→S: {"cmd":"RENAME_DIR","actual":"dir_a","nuevo":"dir_b"}
     *   S→C: {"status":"RENAME_OK"}
     *        o {"status":"ERROR","msg":"..."}
     *
     * [GUI] · actual = fila seleccionada (tipo DIR) en el JTable remoto.
     *       · nuevo  = JOptionPane.showInputDialog con actual pre-cargado.
     *       · Refrescar la lista remota al terminar.
     *
     * @param actual Nombre actual de la carpeta remota.
     * @param nuevo  Nuevo nombre.
     * @return true si fue renombrada correctamente.
     */
    public static boolean renombrarCarpetaRemota(String actual, String nuevo) {
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "RENAME_DIR");
            cmd.addProperty("actual", actual);
            cmd.addProperty("nuevo",  nuevo);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            return esExito(resp);

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error
            return false;
        }
    }


    // =================================================================
    //  SALIR
    // =================================================================

    /**
     * Notifica al servidor que el cliente se desconecta.
     *
     * Protocolo:
     *   C→S: {"cmd":"EXIT"}
     *   (el servidor cierra la conexión por su lado sin responder)
     *
     * [GUI] Llamar desde windowClosing() del JFrame o desde el botón
     *       "Cerrar sesión". Después llamar dispose() en el JFrame y
     *       luego cerrarConexionMetadatos().
     */
    public static void salir() {
        JsonObject cmd = new JsonObject();
        cmd.addProperty("cmd", "EXIT");
        enviarJson(cmd);
    }


    // =================================================================
    //  MÉTODOS AUXILIARES DE TRANSFERENCIA  (internos)
    // =================================================================

    /**
     * Envía un archivo individual al servidor por el canal de datos.
     * Usado internamente por subirCarpeta() para cada archivo.
     *
     * Protocolo (sub-protocolo dentro de UPLOAD_DIR):
     *   C→S: {"cmd":"FILE","nombre":"fotos/img.jpg","tamanio":81920}
     *   S→C: {"status":"OK"}
     *   [canal de datos: bytes crudos]
     *   S→C: {"status":"FILE_OK"}
     *
     * @param nombreRelativo Ruta relativa en el servidor, incluye carpeta padre.
     * @param archivo        Archivo local a enviar.
     */
    private static void enviarArchivoConCanal(String nombreRelativo, File archivo) {
        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",     "FILE");
            cmd.addProperty("nombre",  nombreRelativo);
            cmd.addProperty("tamanio", archivo.length());
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) return;

            Socket canalDatos = abrirCanalDatos();
            if (canalDatos == null) return;

            try (FileInputStream fis = new FileInputStream(archivo);
                 OutputStream    os  = canalDatos.getOutputStream()) {

                byte[] buf = new byte[BUFFER];
                int n;
                while ((n = fis.read(buf)) != -1) os.write(buf, 0, n);
                os.flush();

            } finally {
                cerrarCanalDatos(canalDatos);
            }

            leerRespuesta(); // consume FILE_OK

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error si se desea
        }
    }

    /**
     * Recibe exactamente 'tamanio' bytes del canal de datos y los escribe
     * en el archivo destino. Usado por descargarCarpeta() para cada archivo.
     *
     * @param canalDatos Socket del canal de datos, ya conectado.
     * @param destino    Archivo local donde se guardarán los bytes.
     * @param tamanio    Cantidad exacta de bytes que se esperan recibir.
     */
    private static void recibirArchivoConCanal(Socket canalDatos, File destino, long tamanio) {
        try (InputStream         is  = canalDatos.getInputStream();
             FileOutputStream    fos = new FileOutputStream(destino);
             BufferedOutputStream bos = new BufferedOutputStream(fos)) {

            byte[] buf     = new byte[BUFFER];
            long restantes = tamanio;

            while (restantes > 0) {
                int aLeer = (int) Math.min(BUFFER, restantes);
                int leido = is.read(buf, 0, aLeer);
                if (leido == -1) break;
                bos.write(buf, 0, leido);
                restantes -= leido;
                // [GUI-PROGRESO] publish( (int)((tamanio - restantes) * 100 / tamanio) );
            }
            bos.flush();

        } catch (IOException e) {
            // [GUI] Mostrar mensaje de error si se desea
        }
    }


    // =================================================================
    //  UTILIDADES
    // =================================================================

    /**
     * Crea la carpeta local si no existe. Se llama al arrancar y al
     * cambiar carpetaLocal vía setCarpetaLocal().
     */
    private static void crearCarpetaLocalSiNoExiste() {
        File dir = new File(carpetaLocal);
        if (!dir.exists()) dir.mkdirs();
    }
}