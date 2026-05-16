import java.net.*;
import java.io.*;

/**
 *
 * @author escom
 */
public class Cliente {
    public static void main(String[] args){
     String host= "127.0.0.1";
     int pto = 9999;
     ObjectOutputStream oos = null;
     ObjectInputStream ois = null;
     try{
         Socket cl = new Socket(host,pto);
         System.out.println("Conexión establecida...");
         oos= new ObjectOutputStream(cl.getOutputStream());
         ois = new ObjectInputStream(cl.getInputStream());
         Usuario u = new Usuario("Pepito", "Perez","Juarez","12345",20);
         System.out.println("Enviando objeto");
         oos.writeObject(u);
         oos.flush();
         System.out.println("Preparado para recibir respuesta...");
         
        Usuario u2 = (Usuario)ois.readObject();
        System.out.println("Objeto recibido.. Extrayendo informacion");
        System.out.println("Nombre = "+u2.getNombre());
        System.out.println("Apellido Paterno = "+u2.getApaterno());
        System.out.println("Apellido Materno = "+u2.getAmaterno());
        System.out.println("Password = "+u2.getPwd());
        System.out.println("Edad = "+u2.getEdad());
         
     }catch(Exception e){
         System.err.println(e);
     }//catch
    }//main
}//class
