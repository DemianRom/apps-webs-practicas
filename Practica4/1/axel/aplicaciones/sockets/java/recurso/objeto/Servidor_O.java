import java.net.*;
import java.io.*;

public class Servidor_O {

public static void main(String[] args){
    try{
	ServerSocket ss = new ServerSocket(3000);
	System.out.println("Servidor iniciado");
        for(;;){
            Socket cl = ss.accept();
            ObjectOutputStream oos = new ObjectOutputStream(cl.getOutputStream());
            int[][] x = new int[3][3];
            for(int i=0;i<3;i++)
                for(int j=0; j<3;j++)
                    x[i][j]=1;

            Objeto ob2 = new Objeto(1,2.0f,"tres",x.clone());
            oos.writeObject(ob2);
            oos.flush();
            System.out.println("Cliente conectado.. Enviando objeto con los datos\nX:"+ob2.getX()+" Y:"+ob2.getY()+" Z:"+ob2.getZ() );
         
            for(int i=0;i<x.length;i++){
                for(int j=0;j<x[0].length;j++){
                    System.out.print(x[i][j]+" ");
                }//for
                System.out.println("");
            }//for
            x = new int[3][3];  /* debes regenerar la matriz antes de volver a mandar*/
            for(int i=0;i<3;i++)
                for(int j=0; j<3;j++)
                    x[i][j]=2;

            Objeto ob3 = new Objeto(2,2.0f,"dos",x.clone());
            oos.writeObject(ob3);
            oos.flush();
            System.out.println("Enviando objeto con los datos\nX:"+ob3.getX()+" Y:"+ob3.getY()+" Z:"+ob3.getZ() );
         
            for(int i=0;i<x.length;i++){
                for(int j=0;j<x[0].length;j++){
                    System.out.print(x[i][j]+" ");
                }//for
                System.out.println("");
            }//for


            ObjectInputStream ois = new ObjectInputStream(cl.getInputStream());
            Objeto ob = (Objeto)ois.readObject();
            int[][]y = ob.getM();
            System.out.println("Objeto recibido desde"+cl.getInetAddress()+":"+cl.getPort()+" con los datos");
            System.out.println("x:"+ob.getX()+" y:"+ob.getY()+" Z:"+ob.getZ());
                        System.out.println("M: \n");
            for(int i=0;i<y.length;i++){
                for(int j=0;j<y[0].length;j++){
                    System.out.print(y[i][j]+" ");
                }//for
                System.out.println("");
            }//for

            ois.close();
            oos.close();
            cl.close();
        }//for   
    }catch(Exception e){
        e.printStackTrace();
    }
	}//main
	
}