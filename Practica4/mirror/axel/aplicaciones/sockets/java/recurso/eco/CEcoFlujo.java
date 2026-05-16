import java.net.*;
import java.io.*;
/**
 *
 * @author axele
 */
public class CEcoFlujo {
    public static void main(String[] args){
        try{
            int pto = 8000;
            BufferedReader br = new BufferedReader(new InputStreamReader(System.in,"ISO-8859-1"));//"Windows-1250"
            InetAddress host = null; //Integer.MAX_VALUE
            String dir="";
            try{
                System.out.println("Escribe la direccion del servidor:");
                dir = br.readLine();  //\n\r (13)(10)
                host = InetAddress.getByName(dir);//UnknownHostException
            }catch(Exception n){
               main(args); 
            }
//catch
            Socket cl = new Socket(host,pto);
            cl.setTcpNoDelay(true);
            cl.setOOBInline(true);
           
            //cl.sendUrgentData(5);
            System.out.println("Conexion con el servidor "+dir+":"+pto+" establecida\n");
            PrintWriter pw = new PrintWriter(new OutputStreamWriter(cl.getOutputStream(),"ISO-8859-1"));
            BufferedReader br1 = new BufferedReader(new InputStreamReader(cl.getInputStream(),"ISO-8859-1"));
            while(true){
                System.out.println("Escribe un mensaje, <ENTER> para enviar, \"salir\" para terminar");
                String mensaje = br.readLine();  //Integer.MAX_VALUE
                pw.println(mensaje);  
                pw.flush();
                if(mensaje.compareToIgnoreCase("salir")==0){
                    br.close();
                    br1.close();
                    pw.close();
                    cl.close();
                    System.exit(0);
                } else{
                    String eco = br1.readLine();
                    System.out.println("Eco recibido desde "+cl.getInetAddress()+":"+cl.getPort()+"--> "+eco);
                }//else
            }//while
        }catch(Exception e){
            e.printStackTrace();
        }//catch
    }//main
}
