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
 *  ║            subirArchivo();   // aquí va la llamada de red     ║
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
 *  ║  Esos valores reemplazan SERVER_IP, PUERTO_META y             ║
 *  ║  carpetaLocal antes de llamar a conectarMetadatos().          ║
 *  ║                                                               ║
 *  ║  BUSCA EN EL CÓDIGO:  // [GUI]                                ║
 *  ║  Cada línea marcada con ese comentario indica exactamente      ║
 *  ║  qué entrada de consola o impresión debes reemplazar.         ║
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
    //  [GUI] Estos valores los captura el diálogo de inicio de sesión.
    // -----------------------------------------------------------------
    private static String SERVER_IP    = "127.0.0.1";
    private static int    PUERTO_META  = 8000;
    private static int    PUERTO_DATOS = 8001;

    private static final int BUFFER = 4096; // bloque de lectura/escritura (4 KB)

    // -----------------------------------------------------------------
    //  CANAL DE METADATOS  (permanente durante la sesión)
    // -----------------------------------------------------------------
    private static Socket        socketMeta;
    private static PrintWriter   salida;   // envía JSON al servidor
    private static BufferedReader entrada; // recibe JSON del servidor

    // -----------------------------------------------------------------
    //  CARPETA LOCAL
    //  [GUI] El diálogo de inicio de sesión permite cambiarla.
    // -----------------------------------------------------------------
    private static String carpetaLocal =
            System.getProperty("user.home") + File.separator + "cliente_archivos";

    // -----------------------------------------------------------------
    //  CONSOLA  [GUI] Eliminar este Scanner al agregar la interfaz.
    // -----------------------------------------------------------------
    private static Scanner teclado = new Scanner(System.in);


    // =================================================================
    //  MAIN
    // =================================================================

    public static void main(String[] args) {

        System.out.println("=====================================================");
        System.out.println("  Práctica 1 — Transferencia de archivos  |  CLIENTE");
        System.out.println("=====================================================\n");

        crearCarpetaLocalSiNoExiste();

        if (!conectarMetadatos()) {
            System.out.println("[ERROR] No se pudo conectar al servidor.");
            return;
        }

        // [GUI] Este bucle desaparece. La ventana principal escucha eventos
        //       de botones y cada botón invoca directamente el método
        //       correspondiente (subirArchivo(), descargarArchivo(), etc.)
        //       dentro de un SwingWorker para no bloquear la interfaz.
        boolean sesionActiva = true;
        while (sesionActiva) {
            mostrarMenu();
            int opcion = leerOpcion();

            switch (opcion) {
                case 1  -> mostrarContenidoLocal();
                case 2  -> mostrarContenidoRemoto();
                case 3  -> subirArchivo();
                case 4  -> descargarArchivo();
                case 5  -> subirCarpeta();
                case 6  -> descargarCarpeta();
                case 7  -> borrarArchivoLocal();
                case 8  -> borrarArchivoRemoto();
                case 9  -> renombrarArchivoLocal();
                case 10 -> renombrarArchivoRemoto();
                case 11 -> renombrarCarpetaLocal();
                case 12 -> renombrarCarpetaRemota();
                case 13 -> sesionActiva = salir();
                default -> System.out.println("[AVISO] Opción no válida (1-13).");
            }
        }

        cerrarConexionMetadatos();
    }


    // =================================================================
    //  UTILIDADES JSON
    //  Único punto de construcción y parseo de mensajes.
    //  Si se cambia la librería JSON (Gson → Jackson, org.json, etc.)
    //  solo hay que modificar estas dos funciones y los imports.
    // =================================================================

    /**
     * Serializa el JsonObject y lo envía al servidor como una línea.
     * El \n al final de println() actúa como delimitador de mensaje.
     *
     * Ejemplo de uso:
     *   JsonObject cmd = new JsonObject();
     *   cmd.addProperty("cmd", "DELETE");
     *   cmd.addProperty("nombre", "foto.jpg");
     *   enviarJson(cmd);
     *   → envía:  {"cmd":"DELETE","nombre":"foto.jpg"}\n
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
     * Cualquier otro valor ("OK", "UPLOAD_OK", "DELETE_OK", etc.) es éxito.
     */
    private static boolean esExito(JsonObject resp) {
        if (resp == null) return false;
        return !resp.get("status").getAsString().equals("ERROR");
    }


    // =================================================================
    //  CONEXIÓN Y DESCONEXIÓN
    // =================================================================

    /**
     * Abre el canal de METADATOS (puerto 8000).
     *
     * PrintWriter con autoFlush=true: cada println() envía los datos
     * de inmediato sin esperar a que el buffer se llene. Es obligatorio
     * en un protocolo pregunta-respuesta donde el servidor espera el
     * mensaje completo antes de responder.
     *
     * [GUI] Llamar este método desde el botón "Conectar" del diálogo
     *       de inicio de sesión, después de leer SERVER_IP y PUERTO_META.
     *
     * @return true si la conexión fue exitosa.
     */
    private static boolean conectarMetadatos() {
        try {
            System.out.println("[INFO] Conectando a " + SERVER_IP + ":" + PUERTO_META + " ...");
            socketMeta = new Socket(SERVER_IP, PUERTO_META);

            salida  = new PrintWriter(
                    new BufferedWriter(
                            new OutputStreamWriter(socketMeta.getOutputStream())), true);

            entrada = new BufferedReader(
                    new InputStreamReader(socketMeta.getInputStream()));

            System.out.println("[INFO] Canal de metadatos listo.\n");
            return true;

        } catch (IOException e) {
            System.out.println("[ERROR] conectarMetadatos(): " + e.getMessage());
            return false;
        }
    }

    /**
     * Cierra el canal de metadatos. Se llama una sola vez al terminar la sesión.
     *
     * [GUI] Llamar desde windowClosing() del JFrame principal.
     */
    private static void cerrarConexionMetadatos() {
        try {
            if (socketMeta != null && !socketMeta.isClosed()) socketMeta.close();
            System.out.println("[INFO] Sesión cerrada.");
        } catch (IOException e) {
            System.out.println("[ERROR] cerrarConexionMetadatos(): " + e.getMessage());
        }
    }

    /**
     * Abre un socket nuevo en el canal de DATOS (puerto 8001).
     *
     * Intermitente: existe solo durante la transferencia de bytes.
     * El servidor tiene un ServerSocket.accept() esperando cada vez
     * que se abre este canal.
     *
     * @return Socket abierto, o null si falló la conexión.
     */
    private static Socket abrirCanalDatos() {
        try {
            Socket s = new Socket(SERVER_IP, PUERTO_DATOS);
            System.out.println("[INFO] Canal de datos abierto.");
            return s;
        } catch (IOException e) {
            System.out.println("[ERROR] abrirCanalDatos(): " + e.getMessage());
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
            if (s != null && !s.isClosed()) {
                s.close();
                System.out.println("[INFO] Canal de datos cerrado.");
            }
        } catch (IOException e) {
            System.out.println("[ERROR] cerrarCanalDatos(): " + e.getMessage());
        }
    }


    // =================================================================
    //  MENÚ DE CONSOLA
    //  [GUI] TODO este bloque se elimina por completo al agregar la GUI.
    //        Cada opción del menú se convierte en un botón o acción
    //        en la ventana principal (ver guía al inicio del archivo).
    // =================================================================

    private static void mostrarMenu() {
        System.out.println("\n----------------------------------------------");
        System.out.println("  MENÚ  |  Carpeta local: " + carpetaLocal);
        System.out.println("----------------------------------------------");
        System.out.println("  1.  Listar carpeta LOCAL");
        System.out.println("  2.  Listar carpeta REMOTA");
        System.out.println("  3.  Subir archivo");
        System.out.println("  4.  Descargar archivo");
        System.out.println("  5.  Subir carpeta");
        System.out.println("  6.  Descargar carpeta");
        System.out.println("  7.  Borrar archivo LOCAL");
        System.out.println("  8.  Borrar archivo REMOTO");
        System.out.println("  9.  Renombrar archivo LOCAL");
        System.out.println("  10. Renombrar archivo REMOTO");
        System.out.println("  11. Renombrar carpeta LOCAL");
        System.out.println("  12. Renombrar carpeta REMOTA");
        System.out.println("  13. Salir");
        System.out.println("----------------------------------------------");
        System.out.print("  Opción: ");
    }

    // [GUI] Eliminar — el valor viene del botón presionado.
    private static int leerOpcion() {
        try { return Integer.parseInt(teclado.nextLine().trim()); }
        catch (NumberFormatException e) { return -1; }
    }

    /**
     * Lee una cadena desde consola.
     *
     * [GUI] Reemplazar según el caso:
     *   · Nombre de archivo  → JTextField o JFileChooser
     *   · Nombre de carpeta  → JFileChooser.setFileSelectionMode(DIRECTORIES_ONLY)
     *   · Nuevo nombre       → JOptionPane.showInputDialog(frame, "Nuevo nombre:")
     */
    private static String pedirTexto(String mensaje) {
        System.out.print("  " + mensaje + ": ");
        return teclado.nextLine().trim();
    }


    // =================================================================
    //  OPCIÓN 1 — LISTAR CARPETA LOCAL  (sin red)
    // =================================================================

    /**
     * Lista archivos y subdirectorios de la carpeta local.
     * Operación 100 % local — no usa la red en ningún momento.
     *
     * [GUI] Poblar el JTable del panel izquierdo con el arreglo File[].
     *       Llamar automáticamente al arrancar y después de cualquier
     *       operación local (borrar, renombrar, descargar).
     *       Columnas sugeridas: Nombre | Tipo | Tamaño | Fecha de modificación.
     */
    private static void mostrarContenidoLocal() {
        System.out.println("\n[LOCAL] " + carpetaLocal);
        File[] items = new File(carpetaLocal).listFiles();

        if (items == null || items.length == 0) {
            System.out.println("  (vacío)");
            return;
        }
        for (File f : items) {
            System.out.printf("  %s  %-40s  %,d bytes%n",
                    f.isDirectory() ? "[DIR ]" : "[FILE]",
                    f.getName(),
                    f.length());
        }
    }


    // =================================================================
    //  OPCIÓN 2 — LISTAR CARPETA REMOTA
    // =================================================================

    /**
     * Pide al servidor su lista de archivos y la imprime.
     *
     * Protocolo:
     *   C→S: {"cmd":"LIST"}
     *   S→C: {"status":"ITEM","tipo":"FILE","nombre":"x","tamanio":100}
     *        {"status":"ITEM","tipo":"DIR","nombre":"fotos","tamanio":0}
     *        ... (una línea por entrada)
     *        {"status":"FIN_LIST"}
     *
     * ¿Por qué enviar ítems uno a uno en vez de un array JSON?
     *   El servidor puede hacer streaming de los ítems conforme los
     *   lee del disco sin construir todo el array en memoria.
     *   Para carpetas con miles de archivos esto es mucho más eficiente.
     *
     * [GUI] Poblar el JTable del panel derecho con cada ITEM recibido.
     *       Llamar al botón "Actualizar" del panel derecho.
     *       Columnas sugeridas: Nombre | Tipo | Tamaño.
     */
    private static void mostrarContenidoRemoto() {
        try {
            System.out.println("\n[REMOTO] Solicitando lista...");

            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd", "LIST");
            enviarJson(cmd);

            JsonObject resp;
            while ((resp = leerRespuesta()) != null) {
                String status = resp.get("status").getAsString();
                if (status.equals("FIN_LIST")) break;
                if (status.equals("ITEM")) {
                    System.out.printf("  %s  %-40s  %,d bytes%n",
                            resp.get("tipo").getAsString().equals("DIR") ? "[DIR ]" : "[FILE]",
                            resp.get("nombre").getAsString(),
                            resp.get("tamanio").getAsLong());
                }
            }

        } catch (IOException e) {
            System.out.println("[ERROR] mostrarContenidoRemoto(): " + e.getMessage());
        }
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
     * ¿Por qué dos fases (negociación + datos)?
     *   Permite al servidor rechazar antes de abrir el canal de datos
     *   (disco lleno, nombre inválido, permisos, etc.). Así no se
     *   desperdicia una conexión TCP extra.
     *
     * [GUI] · El nombre viene de un JFileChooser (no de un JTextField).
     *       · Mostrar el JFileChooser con currentDirectory = carpetaLocal.
     *       · Ejecutar en SwingWorker para no bloquear la interfaz.
     *       · Actualizar barra de progreso en los puntos [GUI-PROGRESO].
     *       · Al terminar (done()), refrescar la lista remota.
     */
    private static void subirArchivo() {
        // [GUI] nombre = jFileChooser.getSelectedFile().getName()
        String nombre  = pedirTexto("Nombre del archivo a subir");
        File   archivo = new File(carpetaLocal, nombre);

        if (!archivo.exists() || !archivo.isFile()) {
            System.out.println("[AVISO] Archivo no encontrado: " + nombre);
            return;
        }

        long tamanio = archivo.length();

        try {
            // --- Fase 1: negociación ---
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",     "UPLOAD");
            cmd.addProperty("nombre",  nombre);
            cmd.addProperty("tamanio", tamanio);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) {
                System.out.println("[ERROR] Servidor: " + resp.get("msg").getAsString());
                return;
            }

            // --- Fase 2: transferencia ---
            Socket canalDatos = abrirCanalDatos();
            if (canalDatos == null) return;

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
                System.out.println("[INFO] Enviados: " + totalEnviado + " bytes");

            } finally {
                cerrarCanalDatos(canalDatos);
            }

            // --- Fase 3: confirmación ---
            JsonObject conf = leerRespuesta();
            if (conf != null && conf.get("status").getAsString().equals("UPLOAD_OK")) {
                System.out.println("[OK] '" + nombre + "' subido correctamente.");
            } else {
                System.out.println("[ERROR] Confirmación inesperada: " + conf);
            }

        } catch (IOException e) {
            System.out.println("[ERROR] subirArchivo(): " + e.getMessage());
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
     * ¿Por qué leer EXACTAMENTE tamanio bytes y no hasta EOF?
     *   TCP es un flujo continuo: el socket no sabe dónde termina
     *   un archivo y empieza el siguiente mensaje. Si se lee hasta
     *   EOF hay que cerrar el socket para señalar el fin, pero
     *   entonces ya no sirve para el siguiente archivo. Leyendo
     *   exactamente tamanio bytes el canal queda libre para reutilizar.
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable remoto.
     *       · Ejecutar en SwingWorker.
     *       · Actualizar barra de progreso en [GUI-PROGRESO].
     *       · Al terminar (done()), refrescar la lista local.
     */
    private static void descargarArchivo() {
        // [GUI] nombre = (String) tablaRemota.getValueAt(filaSeleccionada, colNombre)
        String nombre = pedirTexto("Nombre del archivo a descargar");

        try {
            // --- Fase 1: solicitar ---
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DOWNLOAD");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) {
                System.out.println("[ERROR] " + resp.get("msg").getAsString());
                return;
            }

            long tamanio = resp.get("tamanio").getAsLong();
            System.out.println("[INFO] El servidor enviará " + tamanio + " bytes");

            // --- Fase 2: recibir bytes ---
            Socket canalDatos = abrirCanalDatos();
            if (canalDatos == null) return;

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

            System.out.println("[OK] Guardado en: " + destino.getAbsolutePath());

        } catch (IOException e) {
            System.out.println("[ERROR] descargarArchivo(): " + e.getMessage());
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
     * Se envía la cantidad primero para que el servidor sepa exactamente
     * cuántos archivos esperar sin necesitar un token de fin de carpeta.
     *
     * [GUI] · JFileChooser en modo DIRECTORIES_ONLY.
     *       · Ejecutar en SwingWorker.
     *       · Al terminar (done()), refrescar la lista remota.
     */
    private static void subirCarpeta() {
        // [GUI] jFileChooser.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY)
        //       nombre = jFileChooser.getSelectedFile().getName()
        String nombre  = pedirTexto("Nombre de la carpeta a subir");
        File   carpeta = new File(carpetaLocal, nombre);

        if (!carpeta.exists() || !carpeta.isDirectory()) {
            System.out.println("[AVISO] Carpeta no encontrada: " + nombre);
            return;
        }

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
            if (!esExito(resp)) {
                System.out.println("[ERROR] Servidor: " + resp);
                return;
            }

            for (File archivo : archivos) {
                System.out.println("[INFO] Enviando: " + archivo.getName());
                enviarArchivoConCanal(nombre + "/" + archivo.getName(), archivo);
            }

            System.out.println("[OK] Carpeta '" + nombre + "' subida ("
                    + archivos.size() + " archivo(s)).");

        } catch (IOException e) {
            System.out.println("[ERROR] subirCarpeta(): " + e.getMessage());
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
     * ¿Por qué NEXT_OK después de cada archivo?
     *   El servidor necesita saber que el cliente terminó antes de
     *   enviar los metadatos del siguiente archivo. Sin este ACK,
     *   el servidor podría enviar el JSON del próximo archivo mientras
     *   el cliente todavía está leyendo bytes del canal de datos,
     *   mezclando los dos flujos.
     *
     * [GUI] · nombre viene de la fila seleccionada en el JTable remoto.
     *       · Ejecutar en SwingWorker.
     *       · Al terminar (done()), refrescar la lista local.
     */
    private static void descargarCarpeta() {
        // [GUI] nombre = (String) tablaRemota.getValueAt(filaSeleccionada, colNombre)
        String nombre = pedirTexto("Nombre de la carpeta a descargar");

        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DOWNLOAD_DIR");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            if (!esExito(resp)) {
                System.out.println("[ERROR] " + resp.get("msg").getAsString());
                return;
            }

            int cantidad = resp.get("cantidad").getAsInt();
            System.out.println("[INFO] El servidor enviará " + cantidad + " archivo(s).");

            File carpetaDest = new File(carpetaLocal, nombre);
            carpetaDest.mkdirs();

            for (int i = 0; i < cantidad; i++) {
                JsonObject meta      = leerRespuesta();
                String     nombreArch = meta.get("nombre").getAsString();
                long       tamanio   = meta.get("tamanio").getAsLong();

                System.out.println("[INFO] Recibiendo: " + nombreArch
                        + "  (" + tamanio + " bytes)");

                Socket canalDatos = abrirCanalDatos();
                if (canalDatos == null) break;

                recibirArchivoConCanal(canalDatos, new File(carpetaDest, nombreArch), tamanio);
                cerrarCanalDatos(canalDatos);

                JsonObject ack = new JsonObject();
                ack.addProperty("cmd", "NEXT_OK");
                enviarJson(ack);
            }

            System.out.println("[OK] Carpeta descargada en: " + carpetaDest.getAbsolutePath());

        } catch (IOException e) {
            System.out.println("[ERROR] descargarCarpeta(): " + e.getMessage());
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
     *       · Mostrar JOptionPane.showConfirmDialog antes de borrar.
     *       · Al terminar, refrescar la lista local.
     */
    private static void borrarArchivoLocal() {
        // [GUI] nombre = (String) tablaLocal.getValueAt(filaSeleccionada, colNombre)
        String nombre  = pedirTexto("Nombre del archivo local a borrar");
        File   archivo = new File(carpetaLocal, nombre);

        if (!archivo.exists() || !archivo.isFile()) {
            System.out.println("[AVISO] No encontrado: " + nombre);
            return;
        }
        System.out.println(archivo.delete()
                ? "[OK] '" + nombre + "' eliminado localmente."
                : "[ERROR] No se pudo eliminar (¿en uso?).");
    }

    /**
     * Renombra un archivo en la carpeta local.
     * Operación 100 % local, no usa la red.
     *
     * [GUI] · Abrir un JOptionPane.showInputDialog con el nombre actual
     *         pre-cargado para que el usuario solo edite lo que cambia.
     *       · Al terminar, refrescar la lista local.
     */
    private static void renombrarArchivoLocal() {
        // [GUI] actual = fila seleccionada; nuevo = JOptionPane.showInputDialog(...)
        String actual = pedirTexto("Nombre actual del archivo local");
        String nuevo  = pedirTexto("Nuevo nombre");

        File origen = new File(carpetaLocal, actual);
        File dest   = new File(carpetaLocal, nuevo);

        if (!origen.exists() || !origen.isFile()) {
            System.out.println("[AVISO] No encontrado: " + actual);
            return;
        }
        System.out.println(origen.renameTo(dest)
                ? "[OK] '" + actual + "' → '" + nuevo + "'"
                : "[ERROR] No se pudo renombrar.");
    }

    /**
     * Renombra una carpeta en la carpeta local.
     * Operación 100 % local, no usa la red.
     *
     * [GUI] Igual que renombrarArchivoLocal pero la fila seleccionada
     *       debe ser de tipo DIR. Refrescar la lista local al terminar.
     */
    private static void renombrarCarpetaLocal() {
        // [GUI] actual = fila seleccionada (tipo DIR); nuevo = JOptionPane.showInputDialog(...)
        String actual = pedirTexto("Nombre actual de la carpeta local");
        String nuevo  = pedirTexto("Nuevo nombre");

        File origen = new File(carpetaLocal, actual);
        File dest   = new File(carpetaLocal, nuevo);

        if (!origen.exists() || !origen.isDirectory()) {
            System.out.println("[AVISO] No encontrado: " + actual);
            return;
        }
        System.out.println(origen.renameTo(dest)
                ? "[OK] Carpeta '" + actual + "' → '" + nuevo + "'"
                : "[ERROR] No se pudo renombrar.");
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
     *       · Mostrar JOptionPane.showConfirmDialog antes de enviar.
     *       · Al terminar, refrescar la lista remota.
     */
    private static void borrarArchivoRemoto() {
        // [GUI] nombre = (String) tablaRemota.getValueAt(filaSeleccionada, colNombre)
        String nombre = pedirTexto("Nombre del archivo remoto a borrar");

        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "DELETE");
            cmd.addProperty("nombre", nombre);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            System.out.println(
                    resp != null && resp.get("status").getAsString().equals("DELETE_OK")
                            ? "[OK] '" + nombre + "' eliminado del servidor."
                            : "[ERROR] " + resp);
        } catch (IOException e) {
            System.out.println("[ERROR] borrarArchivoRemoto(): " + e.getMessage());
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
     *       · Al terminar, refrescar la lista remota.
     */
    private static void renombrarArchivoRemoto() {
        // [GUI] actual = fila seleccionada; nuevo = JOptionPane.showInputDialog(...)
        String actual = pedirTexto("Nombre actual del archivo remoto");
        String nuevo  = pedirTexto("Nuevo nombre");

        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "RENAME_FILE");
            cmd.addProperty("actual", actual);
            cmd.addProperty("nuevo",  nuevo);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            System.out.println(esExito(resp)
                    ? "[OK] '" + actual + "' → '" + nuevo + "'"
                    : "[ERROR] " + resp.get("msg").getAsString());
        } catch (IOException e) {
            System.out.println("[ERROR] renombrarArchivoRemoto(): " + e.getMessage());
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
     *       · Al terminar, refrescar la lista remota.
     */
    private static void renombrarCarpetaRemota() {
        // [GUI] actual = fila seleccionada (tipo DIR); nuevo = JOptionPane.showInputDialog(...)
        String actual = pedirTexto("Nombre actual de la carpeta remota");
        String nuevo  = pedirTexto("Nuevo nombre");

        try {
            JsonObject cmd = new JsonObject();
            cmd.addProperty("cmd",    "RENAME_DIR");
            cmd.addProperty("actual", actual);
            cmd.addProperty("nuevo",  nuevo);
            enviarJson(cmd);

            JsonObject resp = leerRespuesta();
            System.out.println(esExito(resp)
                    ? "[OK] Carpeta '" + actual + "' → '" + nuevo + "'"
                    : "[ERROR] " + resp.get("msg").getAsString());
        } catch (IOException e) {
            System.out.println("[ERROR] renombrarCarpetaRemota(): " + e.getMessage());
        }
    }


    // =================================================================
    //  OPCIÓN 13 — SALIR
    // =================================================================

    /**
     * Notifica al servidor que el cliente se desconecta.
     *
     * Protocolo:
     *   C→S: {"cmd":"EXIT"}
     *   (el servidor cierra la conexión por su lado sin responder)
     *
     * [GUI] Llamar desde windowClosing() del JFrame o desde el botón
     *       "Cerrar sesión". Después llamar dispose() en el JFrame.
     *
     * @return false — detiene el bucle del menú de consola.
     */
    private static boolean salir() {
        JsonObject cmd = new JsonObject();
        cmd.addProperty("cmd", "EXIT");
        enviarJson(cmd);
        System.out.println("[INFO] Enviando EXIT al servidor...");
        return false;
    }


    // =================================================================
    //  MÉTODOS AUXILIARES DE TRANSFERENCIA
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
     * @param nombreRelativo Ruta relativa en el servidor, incluye carpeta padre
     *                       ("fotos/imagen.jpg"). El servidor usa esto para saber
     *                       en qué subdirectorio guardar el archivo.
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
            if (!esExito(resp)) {
                System.out.println("[ERROR] Servidor no listo para: " + nombreRelativo);
                return;
            }

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

            JsonObject conf = leerRespuesta();
            if (conf == null || !conf.get("status").getAsString().equals("FILE_OK"))
                System.out.println("[AVISO] Respuesta inesperada: " + conf);

        } catch (IOException e) {
            System.out.println("[ERROR] enviarArchivoConCanal(): " + e.getMessage());
        }
    }

    /**
     * Recibe exactamente 'tamanio' bytes del canal de datos y los escribe
     * en el archivo destino. Usado por descargarCarpeta() para cada archivo.
     *
     * Ver descargarArchivo() para la explicación de por qué se lee
     * exactamente tamanio bytes y no hasta EOF.
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
            System.out.println("[ERROR] recibirArchivoConCanal(): " + e.getMessage());
        }
    }


    // =================================================================
    //  UTILIDADES
    // =================================================================

    /**
     * Crea la carpeta local si no existe. Se llama una sola vez al arrancar.
     *
     * [GUI] En la primera ejecución mostrar un JFileChooser preguntando
     *       dónde quiere el usuario su carpeta local y guardar la ruta
     *       elegida en carpetaLocal antes de llamar a este método.
     */
    private static void crearCarpetaLocalSiNoExiste() {
        File dir = new File(carpetaLocal);
        if (!dir.exists()) {
            dir.mkdirs();
            System.out.println("[INFO] Carpeta local creada: " + carpetaLocal);
        } else {
            System.out.println("[INFO] Carpeta local: " + carpetaLocal);
        }
    }
}