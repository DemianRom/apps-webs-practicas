import java.net.*;
import java.io.*;
/**
 *
 * @author escom
 */
public class Servidor {
    public static void main(String[] args){
        ObjectOutputStream oos = null;
        ObjectInputStream ois= null;
      try{
        ServerSocket s = new ServerSocket(9999);
        System.out.println("Servicio iniciado... Esperando cliente");
        for(;;){
            Socket cl= s.accept();
            System.out.println("Cliente conectado desde "+cl.getInetAddress()+":"+cl.getPort());
            oos= new ObjectOutputStream(cl.getOutputStream());
            ois = new ObjectInputStream(cl.getInputStream());
            Usuario u = (Usuario)ois.readObject();
            System.out.println("Objeto recibido.. Extrayendo informacion");
            System.out.println("Nombre = "+u.getNombre());
            System.out.println("Apellido Paterno = "+u.getApaterno());
            System.out.println("Apellido Materno = "+u.getAmaterno());
            System.out.println("Password = "+u.getPwd());
            System.out.println("Edad = "+u.getEdad());
            System.out.println("Devolviendo objeto...");
            oos.writeObject(u);
            oos.flush();
            
        }//for
        
      }  catch(Exception e){
          e.printStackTrace();
      }//catch
    }//main
}//class
