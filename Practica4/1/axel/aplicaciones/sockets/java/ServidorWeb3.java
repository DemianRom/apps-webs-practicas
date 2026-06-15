import java.net.*;
import java.io.*;
import java.util.*;

public class ServidorWeb3{
	public static final int PUERTO=80;
	ServerSocket ss;
	
/**
 *Clase interna Manejador
 *se define el comportamiento decada hilo
 *
 */
	class Manejador extends Thread{
    	protected Socket socket;
    	protected PrintWriter pw;
    	protected BufferedOutputStream bos;
    	protected BufferedReader br;
    	
    	public Manejador(Socket _socket) throws Exception{
     	 this.socket=_socket;     	
    	}//constructor
    	
    	/**
    	 *Metodo Run
    	 *define las acciones decada hilo
    	 *
    	 */
    	 public void run(){
      		try{
				// Prepara nuestros readers and writers
				br = new BufferedReader(new InputStreamReader(socket.getInputStream()));
				bos = new BufferedOutputStream(socket.getOutputStream());
				pw = new PrintWriter(new OutputStreamWriter(bos));
				//socket.setSoKeepAlive(true);

				// Lee la peticion HTTP del usuario(hopefully GET /file...... )
				String line = br.readLine();
				// Shutdown any further input
				//socket.shutdownInput();
				if(line == null){
					pw.print("<html><head><title>Servidor web ");
	  				pw.print("</title> <BODY bgcolor=\"#AACCFF\"> <br>Linea vacia</br>");
	  				pw.print("</BODY><html>");
	  				socket.close();
	  				return;
				}//if
				System.out.println("\nCliente conectado desde:"+ socket.getInetAddress());
				System.out.println("Por el puerto:"+ socket.getPort());
				System.out.println("Datos:"+ line +"\r\n\r\n");
				/*****/
				if(line.indexOf("?")== -1){
					pw.print("<html><head><title>Servidor web </title></head>");
					pw.flush();
					pw.print("<BODY bgcolor=\"#AACCFF\"> <center><br><h1>Linea sin parametros</h1></br>");
					pw.print("<form name=\"forma1\" action=\"http://148.204.187.100\" method=\"GET\">");
					pw.flush();
					pw.print("<font size=4 color=#FFFFFF>");
					pw.flush();
					pw.print("<table><tr><td><b>Nombre(s)</b></td><td><input type=\"text\" name=\"nombre\"></td></tr>");
	  				pw.flush();
	  				pw.print("<tr><td><b>Apellido paterno</b></td><td><input type=\"text\" name=\"apaterno\"></td></tr>");
	  				pw.flush();
	  				pw.print("<tr><td><b>Apellido materno</b></td><td><input type=\"text\" name=\"amaterno\"></td></tr>");
	  				pw.flush();
	  				pw.print("</font>");
	  				pw.flush();
	  				pw.print("</table><input type=\"submit\" value=\"Enviar\"></center>");
	  				pw.print("<center><a href=\"148.204.187.100\"><img src=\"file://d:\\axl\\redes2\\duke.gif\"></a></form></center>");
	  				pw.print("</BODY><html>");
	  				pw.flush();
					
					} else if(line.toUpperCase().startsWith("GET")){
	  				// Eliminate any trailing ? data, such as for a CGI
	  				// GET request
	  				StringTokenizer tokens = new StringTokenizer(line,"?");
	  				String req_a = tokens.nextToken();
	  				String req = tokens.nextToken();
	  				System.out.println("Token1:"+ req_a +"\r\n\r\n");
	  				System.out.println("Token2:"+ req +"\r\n\r\n");
	  				pw.println("HTTP/1.0 200 Okay");
   					pw.flush();
   					pw.println();
   					pw.flush();
	  				pw.print("<html><head><title>Servidor web ");
	  				pw.flush();
	  				pw.print("</title> <BODY bgcolor=\"#AACCFF\"> <center><h1><br>Parametros obtenidos..</br></h1>");
	  				pw.flush();
	  				pw.print("<h3><b>"+req+"</b></h3>");
	  				pw.flush();
	  				pw.print("</center></BODY><html>");
	  				pw.flush();
	  		}//if_GET
	  		// If not a GET request, the server will not support it
	  		else{
	    		pw.println("HTTP/1.0 501 Not Implemented");
	    		pw.println();
	  		}//else
	  		pw.flush();
	  		bos.flush();
			} catch(Exception e){
	  		e.printStackTrace();
			}
			try{
	  		socket.close();
			}catch(Exception e){
	  		e.printStackTrace();
			}
    	}//run	
	}//class_Manejador
	
	public ServidorWeb3() throws Exception{
   		System.out.println ("Iniciando servidor...... ");
   		// Create a new server socket
   		this.ss = new ServerSocket(PUERTO);
   		System.out.println ("Servidor iniciado:---OK");
   		System.out.println("Esperando por cliente.....");
   		for (;;){
			// Accept a new socket connection from our server socket
			Socket accept = ss.accept();
			// Start a new handler instance to process the request
			new Manejador(accept).start();
   		}//for
	}//constructor_WebServerDemo

	// Start an instance of the web server running
	public static void main(String[] args) throws Exception{
   		ServidorWeb3 sWeb = new ServidorWeb3();
	}//main
	
	}//class_ServidorWeb3
//int xx= Integer.parseInt("43");