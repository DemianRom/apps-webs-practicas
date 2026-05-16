import java.io.*;
import java.net.*;

public class Servidor_O_UDP {
  public static void main(String[] args){
      int puerto = 8000;
      DatagramPacket dp= null;
      DatagramSocket s = null;
      //ObjectOutputStream oos=null;
      ObjectInputStream ois = null;
      //ByteArrayInputStream bis;
      Objeto_U objeto =null;
      
      try{
      s = new DatagramSocket(puerto);
      System.out.println("Servidor UDP iniciado en el puerto "+s.getLocalPort());
      System.out.println("Recibiendo datos...");
      for(int i=0;i<10;i++){
      dp = new DatagramPacket(new byte[1024],1024);
      s.receive(dp);
      System.out.println("Datagrama recibido... extrayendo información");
      System.out.println("Host remoto:"+dp.getAddress().getHostAddress()+":"+dp.getPort());
      System.out.println("Datos del paquete:");
      ois = new ObjectInputStream(new ByteArrayInputStream(dp.getData()));
      objeto = (Objeto_U)ois.readObject();
      System.out.println("Nombre: "+objeto.getNombre());
      System.out.println("Edad: "+objeto.getEdad());
      System.out.println("Pwd: "+objeto.getPwd());
      ois.close();
      }//for
      s.close();
      }catch(Exception e){System.err.println(e);}
      System.out.println("Termina el contenido del datagrama...");
  }//main
}//class
