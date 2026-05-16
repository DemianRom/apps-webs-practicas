
import java.io.Serializable;
import java.util.Arrays;


public class Objeto_U implements Serializable {
  int n;
  int total;
  byte[] msj;
  
  
  public Objeto_U(int n, int t, byte[] b){
      this.n = n;
      this.total = t;
      this.msj = Arrays.copyOf(b, b.length);
  }
  
  public int getN(){return this.n;}
  public int getTotal(){return this.total;}
  public byte[] getB(){return this.msj;}
}
