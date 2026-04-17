package practica1;

import javax.swing.*;
import javax.swing.table.DefaultTableModel;
import java.awt.*;
import java.awt.datatransfer.DataFlavor;
import java.awt.dnd.*;
import java.awt.event.*;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.List;

public class ClienteGUI extends JFrame {

    private JTable tablaLocal;
    private JTable tablaRemota;
    private DefaultTableModel modeloLocal;
    private DefaultTableModel modeloRemota;
    private JLabel lblEstado;
    private JProgressBar progressBar;
    
    public ClienteGUI() {
        super("Cliente de Transferencia de Archivos (Retro Edition)");
        setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
        setSize(900, 600);
        setLocationRelativeTo(null);
        
        addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent e) {
                salir();
            }
        });
        
        inicializarComponentes();
        configurarDragAndDrop();
    }

    private void inicializarComponentes() {
        setLayout(new BorderLayout());

        // --- TOOLBAR ---
        JToolBar toolBar = new JToolBar();
        toolBar.setFloatable(false);

        JButton btnSubirArchivo = new JButton("Subir Archivo");
        JButton btnSubirCarpeta = new JButton("Subir Carpeta");
        JButton btnBajarArchivo = new JButton("Descargar");
        JButton btnBorrarLocal = new JButton("Borrar Local");
        JButton btnBorrarRemoto = new JButton("Borrar Remoto");
        JButton btnRenombrarLocal = new JButton("Renombrar Local");
        JButton btnRenombrarRemoto = new JButton("Renombrar Remoto");
        JButton btnActualizar = new JButton("Actualizar");

        toolBar.add(btnSubirArchivo);
        toolBar.add(btnSubirCarpeta);
        toolBar.addSeparator();
        toolBar.add(btnBajarArchivo);
        toolBar.addSeparator();
        toolBar.add(btnBorrarLocal);
        toolBar.add(btnBorrarRemoto);
        toolBar.addSeparator();
        toolBar.add(btnRenombrarLocal);
        toolBar.add(btnRenombrarRemoto);
        toolBar.addSeparator();
        toolBar.add(btnActualizar);

        add(toolBar, BorderLayout.NORTH);

        // --- TABLAS ---
        String[] columnas = {"Nombre", "Tipo", "Tamaño"};
        
        modeloLocal = new DefaultTableModel(columnas, 0) {
            @Override
            public boolean isCellEditable(int row, int column) { return false; }
        };
        tablaLocal = new JTable(modeloLocal);
        
        modeloRemota = new DefaultTableModel(columnas, 0) {
            @Override
            public boolean isCellEditable(int row, int column) { return false; }
        };
        tablaRemota = new JTable(modeloRemota);

        JSplitPane splitPane = new JSplitPane(JSplitPane.HORIZONTAL_SPLIT, 
                new JScrollPane(tablaLocal), new JScrollPane(tablaRemota));
        splitPane.setResizeWeight(0.5);
        
        JPanel pnlListas = new JPanel(new GridLayout(1,2));
        pnlListas.add(new JScrollPane(tablaLocal));
        pnlListas.add(new JScrollPane(tablaRemota));
        
        JPanel pnlSplit = new JPanel(new BorderLayout());
        JLabel lblLoc = new JLabel("Archivos Locales (" + Cliente.getCarpetaLocal() + ")", SwingConstants.CENTER);
        JLabel lblRem = new JLabel("Archivos Remotos", SwingConstants.CENTER);
        
        JPanel pnlTitles = new JPanel(new GridLayout(1,2));
        pnlTitles.add(lblLoc);
        pnlTitles.add(lblRem);
        
        pnlSplit.add(pnlTitles, BorderLayout.NORTH);
        pnlSplit.add(splitPane, BorderLayout.CENTER);

        add(pnlSplit, BorderLayout.CENTER);

        // --- BARRA DE ESTADO ---
        JPanel pnlEstado = new JPanel(new BorderLayout());
        lblEstado = new JLabel(" Listo.");
        progressBar = new JProgressBar(0, 100);
        progressBar.setStringPainted(true);
        progressBar.setVisible(false);
        
        pnlEstado.add(lblEstado, BorderLayout.CENTER);
        pnlEstado.add(progressBar, BorderLayout.EAST);
        add(pnlEstado, BorderLayout.SOUTH);

        // --- EVENTOS ---
        btnActualizar.addActionListener(e -> refrescarListas());
        
        btnSubirArchivo.addActionListener(e -> accionSubirArchivo());
        btnSubirCarpeta.addActionListener(e -> accionSubirCarpeta());
        btnBajarArchivo.addActionListener(e -> accionDescargar());
        
        btnBorrarLocal.addActionListener(e -> accionBorrarLocal());
        btnBorrarRemoto.addActionListener(e -> accionBorrarRemoto());
        
        btnRenombrarLocal.addActionListener(e -> accionRenombrarLocal());
        btnRenombrarRemoto.addActionListener(e -> accionRenombrarRemoto());
        
        // Doble click para renombrar
        tablaLocal.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent me) {
                if(me.getClickCount() == 2) accionRenombrarLocal();
            }
        });
        tablaRemota.addMouseListener(new MouseAdapter() {
            public void mouseClicked(MouseEvent me) {
                if(me.getClickCount() == 2) accionRenombrarRemoto();
            }
        });
    }

    private void configurarDragAndDrop() {
        // Permitir arrastrar archivos a la tabla remota (o a toda la ventana) para subirlos.
        setTransferHandler(new TransferHandler() {
            @Override
            public boolean canImport(TransferSupport support) {
                return support.isDataFlavorSupported(DataFlavor.javaFileListFlavor);
            }

            @Override
            public boolean importData(TransferSupport support) {
                if (!canImport(support)) return false;
                try {
                    List<File> files = (List<File>) support.getTransferable().getTransferData(DataFlavor.javaFileListFlavor);
                    for (File f : files) {
                        subirArchivoDrop(f);
                    }
                    return true;
                } catch (Exception ex) {
                    ex.printStackTrace();
                    return false;
                }
            }
        });
    }
    
    private void subirArchivoDrop(File origen) {
        // Como 'Cliente.subirArchivo' asume que el archivo está en 'carpetaLocal',
        // si viene de afuera, lo copiamos a la carpeta local primero.
        File destino = new File(Cliente.getCarpetaLocal(), origen.getName());
        if (!origen.getAbsolutePath().equals(destino.getAbsolutePath())) {
            try {
                Files.copy(origen.toPath(), destino.toPath(), StandardCopyOption.REPLACE_EXISTING);
            } catch (Exception e) {
                e.printStackTrace();
                lblEstado.setText(" Error al copiar archivo localmente.");
                return;
            }
            refrescarLocal();
        }
        
        if (origen.isDirectory()) {
            accionSubirCarpetaWorker(destino.getName());
        } else {
            accionSubirArchivoWorker(destino.getName());
        }
    }

    public void refrescarListas() {
        refrescarLocal();
        refrescarRemoto();
    }

    private void refrescarLocal() {
        modeloLocal.setRowCount(0);
        File[] archivos = Cliente.listarLocal();
        for (File f : archivos) {
            String tipo = f.isDirectory() ? "DIR" : "FILE";
            modeloLocal.addRow(new Object[]{f.getName(), tipo, f.length()});
        }
    }

    private void refrescarRemoto() {
        lblEstado.setText(" Obteniendo archivos remotos...");
        progressBar.setIndeterminate(true);
        progressBar.setVisible(true);
        
        new SwingWorker<List<Cliente.ItemRemoto>, Void>() {
            @Override
            protected List<Cliente.ItemRemoto> doInBackground() {
                return Cliente.listarRemoto();
            }

            @Override
            protected void done() {
                try {
                    List<Cliente.ItemRemoto> archivos = get();
                    modeloRemota.setRowCount(0);
                    for (Cliente.ItemRemoto item : archivos) {
                        modeloRemota.addRow(new Object[]{item.nombre, item.tipo, item.tamanio});
                    }
                    lblEstado.setText(" Lista remota actualizada.");
                } catch (Exception e) {
                    lblEstado.setText(" Error al listar remotos.");
                } finally {
                    progressBar.setIndeterminate(false);
                    progressBar.setVisible(false);
                }
            }
        }.execute();
    }

    // --- ACCIONES CON SWINGWORKER ---

    private void accionSubirArchivo() {
        JFileChooser jfc = new JFileChooser(Cliente.getCarpetaLocal());
        jfc.setDialogTitle("Selecciona un archivo para subir");
        jfc.setFileSelectionMode(JFileChooser.FILES_ONLY);
        if (jfc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            File f = jfc.getSelectedFile();
            // Asegurar que esté en la carpeta local
            if(!f.getParentFile().getAbsolutePath().equals(new File(Cliente.getCarpetaLocal()).getAbsolutePath())){
                JOptionPane.showMessageDialog(this, "Debe seleccionar un archivo directamente dentro de la carpeta local definida.\nPuede usar Arrastrar y Soltar si el archivo está en otro lugar.");
                return;
            }
            accionSubirArchivoWorker(f.getName());
        }
    }

    private void accionSubirArchivoWorker(String nombre) {
        lblEstado.setText(" Subiendo archivo " + nombre + "...");
        progressBar.setValue(0);
        progressBar.setIndeterminate(true); // El cliente original no expone progreso en % facil aquí, así que indeterminate.
        progressBar.setVisible(true);

        new SwingWorker<Boolean, Integer>() {
            @Override
            protected Boolean doInBackground() {
                return Cliente.subirArchivo(nombre);
            }
            @Override
            protected void done() {
                try {
                    boolean ok = get();
                    lblEstado.setText(ok ? " Archivo subido con éxito." : " Error al subir archivo.");
                    refrescarRemoto();
                } catch (Exception e) {
                    lblEstado.setText(" Error crítico: " + e.getMessage());
                } finally {
                    progressBar.setVisible(false);
                }
            }
        }.execute();
    }

    private void accionSubirCarpeta() {
        JFileChooser jfc = new JFileChooser(Cliente.getCarpetaLocal());
        jfc.setDialogTitle("Selecciona una carpeta para subir");
        jfc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
        if (jfc.showOpenDialog(this) == JFileChooser.APPROVE_OPTION) {
            File f = jfc.getSelectedFile();
            if(!f.getParentFile().getAbsolutePath().equals(new File(Cliente.getCarpetaLocal()).getAbsolutePath())){
                JOptionPane.showMessageDialog(this, "Debe seleccionar una carpeta directamente dentro de la carpeta local definida.");
                return;
            }
            accionSubirCarpetaWorker(f.getName());
        }
    }

    private void accionSubirCarpetaWorker(String nombre) {
        lblEstado.setText(" Subiendo carpeta " + nombre + "...");
        progressBar.setIndeterminate(true);
        progressBar.setVisible(true);

        new SwingWorker<Boolean, Void>() {
            @Override
            protected Boolean doInBackground() {
                return Cliente.subirCarpeta(nombre);
            }
            @Override
            protected void done() {
                try {
                    boolean ok = get();
                    lblEstado.setText(ok ? " Carpeta subida con éxito." : " Error al subir carpeta.");
                    refrescarRemoto();
                } catch (Exception e) {
                    lblEstado.setText(" Error al subir carpeta.");
                } finally {
                    progressBar.setVisible(false);
                }
            }
        }.execute();
    }

    private void accionDescargar() {
        int row = tablaRemota.getSelectedRow();
        if (row < 0) {
            JOptionPane.showMessageDialog(this, "Seleccione un archivo de la lista remota a descargar.");
            return;
        }
        String nombre = (String) modeloRemota.getValueAt(row, 0);
        String tipo = (String) modeloRemota.getValueAt(row, 1);
        
        lblEstado.setText(" Descargando " + nombre + "...");
        progressBar.setIndeterminate(true);
        progressBar.setVisible(true);

        new SwingWorker<Boolean, Void>() {
            @Override
            protected Boolean doInBackground() {
                if (tipo.equals("DIR")) return Cliente.descargarCarpeta(nombre);
                else return Cliente.descargarArchivo(nombre);
            }
            @Override
            protected void done() {
                try {
                    boolean ok = get();
                    lblEstado.setText(ok ? " Descarga exitosa." : " Error en descarga.");
                    refrescarLocal();
                } catch (Exception e) {
                    lblEstado.setText(" Error durante la descarga.");
                } finally {
                    progressBar.setVisible(false);
                }
            }
        }.execute();
    }

    private void accionBorrarLocal() {
        int row = tablaLocal.getSelectedRow();
        if (row < 0) return;
        String nombre = (String) modeloLocal.getValueAt(row, 0);
        int resp = JOptionPane.showConfirmDialog(this, "¿Eliminar local '" + nombre + "'?", "Confirmar eliminación", JOptionPane.YES_NO_OPTION);
        if (resp == JOptionPane.YES_OPTION) {
            if (Cliente.borrarArchivoLocal(nombre)) {
                lblEstado.setText(" Elemento local borrado.");
                refrescarLocal();
            } else lblEstado.setText(" Error al borrar local.");
        }
    }

    private void accionBorrarRemoto() {
        int row = tablaRemota.getSelectedRow();
        if (row < 0) return;
        String nombre = (String) modeloRemota.getValueAt(row, 0);
        int resp = JOptionPane.showConfirmDialog(this, "¿Eliminar remoto '" + nombre + "'?", "Confirmar eliminación", JOptionPane.YES_NO_OPTION);
        if (resp == JOptionPane.YES_OPTION) {
            lblEstado.setText(" Borrando remoto...");
            new SwingWorker<Boolean, Void>() {
                @Override
                protected Boolean doInBackground() { return Cliente.borrarArchivoRemoto(nombre); }
                @Override
                protected void done() {
                    try {
                        lblEstado.setText(get() ? " Elemento remoto borrado." : " Error al borrar remoto.");
                        refrescarRemoto();
                    } catch (Exception e) {}
                }
            }.execute();
        }
    }

    private void accionRenombrarLocal() {
        int row = tablaLocal.getSelectedRow();
        if (row < 0) return;
        String nombre = (String) modeloLocal.getValueAt(row, 0);
        String tipo = (String) modeloLocal.getValueAt(row, 1);
        String nuevo = JOptionPane.showInputDialog(this, "Nuevo nombre:", nombre);
        if (nuevo != null && !nuevo.isEmpty() && !nuevo.equals(nombre)) {
            boolean ok;
            if ("DIR".equals(tipo)) ok = Cliente.renombrarCarpetaLocal(nombre, nuevo);
            else ok = Cliente.renombrarArchivoLocal(nombre, nuevo);
            
            if (ok) refrescarLocal();
            else JOptionPane.showMessageDialog(this, "Error al renombrar local.");
        }
    }

    private void accionRenombrarRemoto() {
        int row = tablaRemota.getSelectedRow();
        if (row < 0) return;
        String nombre = (String) modeloRemota.getValueAt(row, 0);
        String tipo = (String) modeloRemota.getValueAt(row, 1);
        String nuevo = JOptionPane.showInputDialog(this, "Nuevo nombre:", nombre);
        
        if (nuevo != null && !nuevo.isEmpty() && !nuevo.equals(nombre)) {
            lblEstado.setText(" Renombrando remoto...");
            new SwingWorker<Boolean, Void>() {
                @Override
                protected Boolean doInBackground() {
                    if ("DIR".equals(tipo)) return Cliente.renombrarCarpetaRemota(nombre, nuevo);
                    else return Cliente.renombrarArchivoRemoto(nombre, nuevo);
                }
                @Override
                protected void done() {
                    try {
                        if (get()) {
                            lblEstado.setText(" Renombrado remoto exitoso.");
                            refrescarRemoto();
                        } else lblEstado.setText(" Error al renombrar remoto.");
                    } catch (Exception e) {}
                }
            }.execute();
        }
    }

    private void salir() {
        Cliente.salir();
        Cliente.cerrarConexionMetadatos();
        dispose();
        System.exit(0);
    }

    // --- LOGIN ---
    public static void mostrarLoginYArrancar() {
        JDialog dialog = new JDialog((Frame)null, "Inicio de Sesión", true);
        dialog.setLayout(new GridBagLayout());
        GridBagConstraints gbc = new GridBagConstraints();
        gbc.insets = new Insets(5,5,5,5);
        gbc.fill = GridBagConstraints.HORIZONTAL;
        
        JTextField txtIp = new JTextField("127.0.0.1", 15);
        JTextField txtPuerto = new JTextField("8000", 10);
        JTextField txtCarpeta = new JTextField(Cliente.getCarpetaLocal(), 20);
        JButton btnExplorar = new JButton("...");
        btnExplorar.addActionListener(e -> {
            JFileChooser jfc = new JFileChooser();
            jfc.setFileSelectionMode(JFileChooser.DIRECTORIES_ONLY);
            if (jfc.showOpenDialog(dialog) == JFileChooser.APPROVE_OPTION) {
                txtCarpeta.setText(jfc.getSelectedFile().getAbsolutePath());
            }
        });

        gbc.gridx=0; gbc.gridy=0; dialog.add(new JLabel("IP Servidor:"), gbc);
        gbc.gridx=1; dialog.add(txtIp, gbc);
        
        gbc.gridx=0; gbc.gridy=1; dialog.add(new JLabel("Puerto:"), gbc);
        gbc.gridx=1; dialog.add(txtPuerto, gbc);
        
        gbc.gridx=0; gbc.gridy=2; dialog.add(new JLabel("Carpeta Local:"), gbc);
        gbc.gridx=1;
        JPanel pnlCarp = new JPanel(new BorderLayout());
        pnlCarp.add(txtCarpeta, BorderLayout.CENTER);
        pnlCarp.add(btnExplorar, BorderLayout.EAST);
        dialog.add(pnlCarp, gbc);

        JButton btnConectar = new JButton("Conectar");
        gbc.gridx=0; gbc.gridy=3; gbc.gridwidth=2; 
        dialog.add(btnConectar, gbc);

        btnConectar.addActionListener(e -> {
            Cliente.setServerIp(txtIp.getText());
            try { Cliente.setPuertoMeta(Integer.parseInt(txtPuerto.getText())); } catch(Exception ex) {}
            Cliente.setCarpetaLocal(txtCarpeta.getText());
            
            if (Cliente.conectarMetadatos()) {
                dialog.dispose();
                ClienteGUI gui = new ClienteGUI();
                gui.setVisible(true);
                gui.refrescarListas();
            } else {
                JOptionPane.showMessageDialog(dialog, "Error al conectar con el servidor.", "Error", JOptionPane.ERROR_MESSAGE);
            }
        });

        // Configurar el LookAndFeel Retro
        try {
            UIManager.setLookAndFeel("com.sun.java.swing.plaf.motif.MotifLookAndFeel");
            SwingUtilities.updateComponentTreeUI(dialog);
        } catch (Exception ex) {
            ex.printStackTrace();
        }

        dialog.pack();
        dialog.setLocationRelativeTo(null);
        dialog.setDefaultCloseOperation(JDialog.DISPOSE_ON_CLOSE);
        dialog.setVisible(true);
    }

    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> mostrarLoginYArrancar());
    }
}
