import java.net.*;
import java.io.*;
/**
 *
 * @author escom
 */
public class Anuncio {
    public static void main(String[] args ){
        InetAddress gpo=null;
        int pto =9876;
        try{
            MulticastSocket s= new MulticastSocket(9876);
            s.setReuseAddress(true);
            s.setTimeToLive(128);
            String msj ="hola";
            byte[] b = msj.getBytes();
            try{
                gpo = InetAddress.getByName("228.1.1.1");
            }catch(UnknownHostException u){
                System.err.println("Direccion no valida");
            }//catch
            s.joinGroup(gpo);
            for(;;){
                DatagramPacket p = new DatagramPacket(b,b.length,gpo,9999);
                s.send(p);
                System.out.println("Enviando mensaje "+msj+ " con un TTL= "+ s.getTimeToLive());
                try{
                    Thread.sleep(3000);
                }catch(InterruptedException ie){}
            }//for
        }catch(Exception e){
            
        }//catch
    }//main
}
