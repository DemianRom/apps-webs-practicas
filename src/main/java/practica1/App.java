package practica1;

public class App {
    public static void main(String[] args) {

        // Configuración básica
        Cliente.setServerIp("127.0.0.1");
        Cliente.setPuertoMeta(8000);
        Cliente.setCarpetaLocal(System.getProperty("user.home") + "/cliente_archivos");

        // Conectar al servidor
        if (!Cliente.conectarMetadatos()) {
            System.out.println("No se pudo conectar al servidor");
            return;
        }

        System.out.println("Conectado correctamente");

        // Solo prueba básica (sin GUI aún)
        var lista = Cliente.listarLocal();

        System.out.println("Archivos locales:");
        for (var f : lista) {
            System.out.println(f.getName());
        }
    }
}