
import java.io.Serializable;


public class Objeto_U implements Serializable {
  String nombre;
  int edad;
  transient String pwd;
  
  public Objeto_U(String nombre, int edad, String pwd){
      this.nombre = nombre;
      this.edad = edad;
      this.pwd = pwd;
  }
  
  public String getNombre(){return this.nombre;}
  public int getEdad(){return this.edad;}
  public String getPwd(){return this.pwd;}
}
