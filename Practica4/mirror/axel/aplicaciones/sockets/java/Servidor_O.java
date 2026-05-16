import java.net.*;
import java.io.*;

public class Servidor_O {

public static void main(String[] args)throws Exception{
	ServerSocket ss = new ServerSocket(3000);
	System.out.println("Servidor iniciado");
	Socket cl = ss.accept();
	ObjectOutputStream oos = new ObjectOutputStream(cl.getOutputStream());
	Objeto ob2 = new Objeto(3,3);
	oos.writeObject(ob2);
	
	ObjectInputStream ois = new ObjectInputStream(cl.getInputStream());
	Objeto ob = (Objeto)ois.readObject();
	System.out.println("Objeto recibido");
	System.out.println("x:"+ob.x+" y:"+ob.y);
	}
	
}