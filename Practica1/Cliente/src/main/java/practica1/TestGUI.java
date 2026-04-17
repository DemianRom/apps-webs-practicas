package practica1;
import java.util.List;
public class TestGUI {
    public static void main(String[] args) {
        Cliente.setServerIp("127.0.0.1");
        Cliente.setPuertoMeta(8000);
        if (Cliente.conectarMetadatos()) {
            System.out.println("Conectado");
            List<Cliente.ItemRemoto> lista = Cliente.listarRemoto();
            for (Cliente.ItemRemoto i : lista) {
                System.out.println(i.nombre + " - " + i.tamanio);
            }
        } else {
            System.out.println("Falla conexion");
        }
    }
}
