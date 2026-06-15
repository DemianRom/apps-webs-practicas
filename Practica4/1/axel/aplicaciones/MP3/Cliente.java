import javax.sound.sampled.*;
import java.io.*;
import java.net.Socket;

public class Cliente {
    public static void main(String[] args) {
        String serverAddress = "localhost"; // Dirección del servidor
        int port = 5000; // Puerto del servidor
        String outputFilePath = "received_song.mp3"; // Ruta donde se guardará el archivo recibido

        try (Socket socket = new Socket(serverAddress, port)) {
            System.out.println("Conectado al servidor.");

            InputStream is = socket.getInputStream();
            FileOutputStream fos = new FileOutputStream(outputFilePath);
            BufferedOutputStream bos = new BufferedOutputStream(fos);

            byte[] buffer = new byte[1024];
            int count;
            while ((count = is.read(buffer)) > 0) {
                bos.write(buffer, 0, count);
            }

            bos.flush();
            System.out.println("Archivo recibido.");
            bos.close();
            is.close();

            // Reproducir el archivo MP3 recibido
            File file = new File(outputFilePath);
            AudioInputStream audioInputStream = AudioSystem.getAudioInputStream(file);
            Clip clip = AudioSystem.getClip();
            clip.open(audioInputStream);
            clip.start();

            // Esperar a que la reproducción termine
            Thread.sleep(clip.getMicrosecondLength() / 1000);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
