package gui;

import practica1.Cliente;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.event.*;
import java.io.File;
import java.util.List;

public class MainGUI extends JFrame {

    private JTable tablaLocal;
    private JTable tablaRemota;

    private DefaultTableModel modeloLocal;
    private DefaultTableModel modeloRemoto;

    private JLabel lblEstado;
    private JProgressBar progressBar;

    private Cliente cliente;

    public MainGUI() {
        setTitle("Cliente de Archivos");
        setSize(900, 500);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setLocationRelativeTo(null);

        cliente = new Cliente();

        inicializarUI();
        mostrarDialogoConexion();

        setVisible(true);
        refrescarLocal();
        refrescarRemoto();
    }

    private void inicializarUI() {
        setLayout(new BorderLayout());

        // TABLAS
        modeloLocal = new DefaultTableModel(new String[]{"Nombre", "Tipo", "Tamaño"}, 0);
        modeloRemoto = new DefaultTableModel(new String[]{"Nombre", "Tipo", "Tamaño"}, 0);

        tablaLocal = new JTable(modeloLocal);
        tablaRemota = new JTable(modeloRemoto);

        JPanel panelLocal = new JPanel(new BorderLayout());
        panelLocal.add(new JLabel("Archivos Locales", SwingConstants.CENTER), BorderLayout.NORTH);
        panelLocal.add(new JScrollPane(tablaLocal), BorderLayout.CENTER);

        JPanel panelRemoto = new JPanel(new BorderLayout());
        panelRemoto.add(new JLabel("Archivos Remotos", SwingConstants.CENTER), BorderLayout.NORTH);
        panelRemoto.add(new JScrollPane(tablaRemota), BorderLayout.CENTER);

        JSplitPane split = new JSplitPane(
            JSplitPane.HORIZONTAL_SPLIT,
            panelLocal,
            panelRemoto
        );

        add(split, BorderLayout.CENTER);

        // BOTONES
        JPanel panelBotones = new JPanel();

        JButton btnSubir = new JButton("↑ Subir");
        JButton btnDescargar = new JButton("↓ Descargar");
        JButton btnActualizar = new JButton("Actualizar");

        panelBotones.add(btnSubir);
        panelBotones.add(btnDescargar);
        panelBotones.add(btnActualizar);

        add(panelBotones, BorderLayout.NORTH);

        // ESTADO + PROGRESO
        JPanel panelEstado = new JPanel(new BorderLayout());

        lblEstado = new JLabel("Listo");
        progressBar = new JProgressBar(0, 100);
        progressBar.setVisible(false);

        panelEstado.add(lblEstado, BorderLayout.WEST);
        panelEstado.add(progressBar, BorderLayout.EAST);

        add(panelEstado, BorderLayout.SOUTH);

        // EVENTOS

        btnActualizar.addActionListener(e -> refrescarRemoto());

        btnSubir.addActionListener(e -> subirArchivo());

        btnDescargar.addActionListener(e -> descargarArchivo());

        // BORRAR con tecla SUPR
        tablaRemota.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent e) {
                if (e.getKeyCode() == KeyEvent.VK_DELETE) {
                    borrarRemoto();
                }
            }
        });

        // RENOMBRAR doble click
        tablaRemota.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() == 2) {
                    renombrarRemoto();
                }
            }
        });
    }

    private void mostrarDialogoConexion() {
        JTextField ip = new JTextField("127.0.0.1");
        JTextField puerto = new JTextField("8000");
        JTextField carpeta = new JTextField();

        JButton btnExplorar = new JButton("Explorar");

        btnExplorar.addActionListener(e -> {
            JFileChooser fc = new JFileChooser();
            fc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            if (fc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
                carpeta.setText(fc.getSelectedFile().getAbsolutePath());
            }
        });

        JPanel panel = new JPanel(new GridLayout(4, 2));
        panel.add(new JLabel("IP:"));
        panel.add(ip);
        panel.add(new JLabel("Puerto:"));
        panel.add(puerto);
        panel.add(new JLabel("Carpeta:"));
        panel.add(carpeta);
        panel.add(new JLabel(""));
        panel.add(btnExplorar);

        int res = JOptionPane.showConfirmDialog(this, panel, "Conectar", JOptionPane.OK_CANCEL_OPTION);

        if (res == JOptionPane.OK_OPTION) {
            cliente.setServerIp(ip.getText());
            cliente.setPuertoMeta(Integer.parseInt(puerto.getText()));
            cliente.setCarpetaLocal(carpeta.getText());
            cliente.conectarMetadatos();
        } else {
            System.exit(0);
        }
    }

    private void refrescarLocal() {
        modeloLocal.setRowCount(0);

        File dir = new File(cliente.getCarpetaLocal());

        if (!dir.exists() || !dir.isDirectory()) {
            lblEstado.setText("Carpeta local inválida");
            return;
        }

        File[] archivos = dir.listFiles();

        if (archivos == null) {
            lblEstado.setText("No se pudieron listar archivos");
            return;
        }

        for (File f : archivos) {
            String tipo = f.isDirectory() ? "Carpeta" : "Archivo";
            long tamanio = f.isFile() ? f.length() : 0;

            modeloLocal.addRow(new Object[]{
                f.getName(),
                tipo,
                tamanio
            });
        }
    }

    private void refrescarRemoto() {
        modeloRemoto.setRowCount(0);

        List<Cliente.ItemRemoto> lista = Cliente.listarRemoto();

        for (Cliente.ItemRemoto item : lista) {
            modeloRemoto.addRow(new Object[]{
                item.nombre,
                item.tipo,
                item.tamanio
            });
        }
    }

    private String getSeleccionRemota() {
        int fila = tablaRemota.getSelectedRow();
        if (fila == -1) return null;
        return (String) modeloRemoto.getValueAt(fila, 0);
    }

    private void subirArchivo() {
        JFileChooser fc = new JFileChooser();

        if (fc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            File f = fc.getSelectedFile();

            progressBar.setVisible(true);
            progressBar.setValue(0);
            lblEstado.setText("Subiendo...");

            new SwingWorker<Void, Integer>() {

                protected Void doInBackground() {
                    cliente.subirArchivo(f.getName());
                    return null;
                }

                protected void done() {
                    progressBar.setVisible(false);
                    lblEstado.setText("Subida completada");
                    refrescarRemoto();
                }

            }.execute();
        }
    }

    private void descargarArchivo() {
        String nombre = getSeleccionRemota();
        if (nombre == null) return;

        progressBar.setVisible(true);
        progressBar.setValue(0);
        lblEstado.setText("Descargando...");

        new SwingWorker<Void, Integer>() {

            protected Void doInBackground() {
                cliente.descargarArchivo(nombre);
                return null;
            }

            protected void done() {
                progressBar.setVisible(false);
                lblEstado.setText("Descarga completada");
                refrescarLocal();
            }

        }.execute();
    }

    private void borrarRemoto() {
        String nombre = getSeleccionRemota();
        if (nombre == null) return;

        int op = JOptionPane.showConfirmDialog(
                this,
                "¿Eliminar '" + nombre + "'?",
                "Confirmar eliminación",
                JOptionPane.YES_NO_OPTION
        );

        if (op == JOptionPane.YES_OPTION) {
            cliente.borrarArchivoRemoto(nombre);
            refrescarRemoto();
        }
    }

    private void renombrarRemoto() {
        String actual = getSeleccionRemota();
        if (actual == null) return;

        String nuevo = JOptionPane.showInputDialog("Nuevo nombre:");

        if (nuevo != null && !nuevo.isEmpty()) {
            cliente.renombrarArchivoRemoto(actual, nuevo);
            refrescarRemoto();
        }
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(MainGUI::new);
    }
}