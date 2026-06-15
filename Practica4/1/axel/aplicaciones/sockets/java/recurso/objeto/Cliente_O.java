import java.net.*;
import java.io.*;

public class Cliente_O {

    public static void main(String[] args) {
        try {
            
            Socket cl = new Socket("localhost", 3000);
            System.out.println("Conexion con servidor exitosa.. preparado para recibir objeto..");
            ObjectOutputStream oos = new ObjectOutputStream(cl.getOutputStream());
            ObjectInputStream ois = new ObjectInputStream(cl.getInputStream());
            Objeto ob2 = (Objeto) ois.readObject();
            /*Object ob = ois.readObject();
            if(ob instanceof Objeto){
                Objeto o = (Objeto)ob;
            }*/
            System.out.println("Objeto recibido desde " + cl.getInetAddress() + ":" + cl.getPort() + " con los datos:");
            System.out.println("x:" + ob2.getX() + " y:" + ob2.getY() + " Z:" + ob2.getZ());
            int[][]y = ob2.getM();
            System.out.println("M: \n");
            for(int i=0;i<y.length;i++){
                for(int j=0;j<y[0].length;j++){
                    System.out.print(y[i][j]+" ");
                }//for
                System.out.println("");
            }//for
            
            Objeto ob3 = (Objeto) ois.readObject();
            System.out.println("Objeto recibido desde " + cl.getInetAddress() + ":" + cl.getPort() + " con los datos:");
            System.out.println("x:" + ob3.getX() + " y:" + ob3.getY() + " Z:" + ob3.getZ());
            int[][]z = ob3.getM();
            System.out.println("M: \n");
            for(int i=0;i<z.length;i++){
                for(int j=0;j<z[0].length;j++){
                    System.out.print(z[i][j]+" ");
                }//for
                System.out.println("");
            }//for


            int[][] x = new int[3][3];
            for(int i=0;i<3;i++)
                for(int j=0; j<3;j++)
                    x[i][j]=9;
            Objeto ob = new Objeto(4, 5.0f, "seis",x.clone());
            System.out.println("Enviando objeto con los datos\nX:" + ob.getX() + " Y:" + ob.getY() + " Z: " + ob.getZ());
            for(int i=0;i<x.length;i++){
                for(int j=0;j<x[0].length;j++){
                    System.out.print(x[i][j]+" ");
                }//for
                System.out.println("");
            }//for

            oos.writeObject(ob);
            oos.flush();
            System.out.println("Objeto enviado..");
            ois.close();
            oos.close();
            cl.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }//main

}
