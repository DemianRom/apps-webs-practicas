import java.io.*;
import java.net.*;

public class Cliente_O_UDP {
  public static void main(String[] args){
      int puerto = 8000;
      DatagramPacket dp= null;
      DatagramSocket c = null;
      ObjectOutputStream oos=null;
      //ObjectInputStream ois = null;
      ByteArrayOutputStream bos=null;
      Objeto_U objeto =null;
      
      try{
      c = new DatagramSocket();
      dp = new DatagramPacket(new byte[1024],1024);
      InetAddress direccion = InetAddress.getByName("127.0.0.1");
      dp.setAddress(direccion);
      dp.setPort(puerto);
      bos = new ByteArrayOutputStream();
      oos = new ObjectOutputStream(bos);
      byte[] buf= new byte[1024];
      objeto = new Objeto_U(1, 1, "12345".getBytes());
      oos.writeObject(objeto);
      oos.flush();
      buf = bos.toByteArray();
      dp.setData(buf);
      c.send(dp);
      System.out.println("Datagrama enviado con los datos:");
      System.out.println("N: "+objeto.getN());
      System.out.println("de un total de: "+objeto.getTotal());
      System.out.println("Msj: "+new String(objeto.getB()));
      oos.close();
      }catch(Exception e){System.err.println(e);}
      System.out.println("Termina el contenido del datagrama...");
  }//main
}//class
