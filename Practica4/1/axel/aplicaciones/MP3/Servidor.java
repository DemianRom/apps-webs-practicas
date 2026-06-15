import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;

public class Servidor {
    public static void main(String[] args) {
        int port = 5000; // Puerto en el que el servidor escuchará
        String filePath = "song.mp3"; // Ruta al archivo MP3

        try (ServerSocket serverSocket = new ServerSocket(port)) {
            System.out.println("Servidor esperando conexiones en el puerto " + port + "...");
            Socket socket = serverSocket.accept();
            System.out.println("Cliente conectado.");

            File file = new File(filePath);
            FileInputStream fis = new FileInputStream(file);
            BufferedInputStream bis = new BufferedInputStream(fis);
            OutputStream os = socket.getOutputStream();

            byte[] buffer = new byte[1024];
            int count;
            while ((count = bis.read(buffer)) > 0) {
                os.write(buffer, 0, count);
            }

            os.flush();
            System.out.println("Archivo enviado.");
            bis.close();
            os.close();
            socket.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
