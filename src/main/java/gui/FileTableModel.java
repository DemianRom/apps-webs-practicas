package gui;

import javax.swing.table.AbstractTableModel;
import java.io.File;
import java.util.ArrayList;
import java.util.List;

public class FileTableModel extends AbstractTableModel {

    private final String[] columnas = {"Nombre", "Tipo", "Tamaño"};
    private List<FileItem> datos = new ArrayList<>();

    public void setDatos(List<FileItem> nuevos) {
        this.datos = nuevos;
        fireTableDataChanged();
    }

    public FileItem getItem(int row) {
        return datos.get(row);
    }

    @Override
    public int getRowCount() {
        return datos.size();
    }

    @Override
    public int getColumnCount() {
        return columnas.length;
    }

    @Override
    public String getColumnName(int col) {
        return columnas[col];
    }

    @Override
    public Object getValueAt(int row, int col) {
        FileItem item = datos.get(row);

        switch (col) {
            case 0: return item.nombre;
            case 1: return item.tipo;
            case 2: return item.tamanio;
            default: return "";
        }
    }

    // Clase interna
    public static class FileItem {
        public String nombre;
        public String tipo;
        public long tamanio;

        public FileItem(String n, String t, long tam) {
            nombre = n;
            tipo = t;
            tamanio = tam;
        }
    }
}