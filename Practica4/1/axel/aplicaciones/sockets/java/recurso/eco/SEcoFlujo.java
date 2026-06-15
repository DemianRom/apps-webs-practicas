import java.net.*;
import java.io.*;
/**
 *
 * @author axele
 */
public class SEcoFlujo {
    public static void main(String[] args){
        try{
            int pto =8000;
            //InetAddress salida= InetAddress.getByName("192.168.1.71");
            //ServerSocket s = new ServerSocket(pto,1024,salida);
            ServerSocket s = new ServerSocket(pto);
            System.out.println("Servidor iniciado en el puerto "+pto+" .. esperando cliente..");
            for(;;){
                Socket cl = s.accept();
                System.out.println("Cliente conectado desde "+cl.getInetAddress()+":"+cl.getPort());
                PrintWriter pw = new PrintWriter(new OutputStreamWriter(cl.getOutputStream(),"ISO-8859-1"));
                BufferedReader br = new BufferedReader(new InputStreamReader(cl.getInputStream(),"ISO-8859-1"));
                while(true){
                    String msj = br.readLine(); //  \n\r (10)(13)
                    if(msj.compareToIgnoreCase("salir")==0){
                        System.out.println("Cliente cierra conexion");
                        br.close();
                        pw.close();
                        cl.close();
                        break;
                    } else{
                        System.out.println("Mensaje recibido: "+msj+" devolviendo eco");
                        pw.println(msj);
                        pw.flush();
                    }//else
                }//while
            }//for
        }catch(Exception e){
            e.printStackTrace();
        }
    }//main
}
