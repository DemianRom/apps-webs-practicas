import java.net.*;
import java.io.*;

public class Cliente_O {

public static void main(String[] args)throws Exception{
	Socket cl = new Socket("localhost",3000);
	System.out.println("Conexion con servidor exitosa");
	ObjectInputStream ois = new ObjectInputStream(cl.getInputStream());
	Objeto ob2 = (Objeto)ois.readObject();
	System.out.println("x:"+ob2.x+" y:"+ob2.y);
	
	ObjectOutputStream oos = new ObjectOutputStream(cl.getOutputStream());
	//ObjectInputStream ois = new ObjectInputStream(cl.getInputStream());
	Objeto ob = new Objeto(5,4);
	oos.writeObject(ob);
	
	}
	
}