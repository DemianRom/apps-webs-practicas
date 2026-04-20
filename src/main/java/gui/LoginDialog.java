package gui;

import javax.swing.*;
import java.awt.*;

public class LoginDialog extends JDialog {

    private JTextField ipField;
    private JTextField puertoField;
    private boolean conectado = false;

    public LoginDialog(Frame parent) {
        super(parent, "Conectar al servidor", true);

        setLayout(new GridLayout(3, 2));

        add(new JLabel("IP:"));
        ipField = new JTextField("127.0.0.1");
        add(ipField);

        add(new JLabel("Puerto:"));
        puertoField = new JTextField("8000");
        add(puertoField);

        JButton conectarBtn = new JButton("Conectar");
        add(new JLabel());
        add(conectarBtn);

        conectarBtn.addActionListener(e -> {
            conectado = true;
            setVisible(false);
        });

        setSize(300, 150);
        setLocationRelativeTo(parent);
    }

    public String getIP() {
        return ipField.getText();
    }

    public int getPuerto() {
        return Integer.parseInt(puertoField.getText());
    }

    public boolean isConectado() {
        return conectado;
    }
}