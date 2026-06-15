import java.net.*;
import java.io.*;


public class Client2 {
public static void main(String[] args){
try{
   Socket cl = new Socket("148.204.58.221",8000);
   PrintWriter pw = new PrintWriter(new OutputStreamWriter(cl.getOutputStream()));
   BufferedReader br = new BufferedReader(new InputStreamReader(cl.getInputStream()));
   BufferedReader br2 = new BufferedReader(new InputStreamReader(System.in));
   
   System.out.print("Escribe un mensaje: ");
   String msj = br2.readLine();
   pw.println(msj);
   pw.flush();
 
   String acuse = br.readLine();
   System.out.println("acuse: "+acuse);
   br2.close();
   br.close();
   pw.close();
   cl.close();
}catch(Exception e){e.printStackTrace();}
}//main    
}
