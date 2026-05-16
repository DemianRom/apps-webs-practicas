import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
 
import org.snmp4j.CommunityTarget;
import org.snmp4j.Snmp;
import org.snmp4j.TransportMapping;
import org.snmp4j.mp.SnmpConstants;
import org.snmp4j.smi.Address;
import org.snmp4j.smi.GenericAddress;
import org.snmp4j.smi.OID;
import org.snmp4j.smi.OctetString;
import org.snmp4j.smi.VariableBinding;
import org.snmp4j.transport.DefaultUdpTransportMapping;
import org.snmp4j.util.DefaultPDUFactory;
import org.snmp4j.util.TreeUtils;
import org.snmp4j.util.TreeEvent;
 
public class Snmpwalk {  
  private String targetAddr;
  private String oidStr;
  private String commStr;
  private int snmpVersion;
  private String portNum;  
  private String usage;
  private ArrayList<String> results = new ArrayList<String>();
    
  Snmpwalk() throws IOException{
    // Set default value.
    targetAddr = null;
    oidStr = null;
    commStr = "public";
    snmpVersion = SnmpConstants.version2c;
    portNum =  "161";
    usage = "Usage: snmpwalk [-c communityName -p portNumber -v snmpVersion(1 or 2)] targetAddr oid";
  }
  
  public Snmpwalk(HashMap<String, String> values){
      targetAddr = values.get("ipaddr");
	  oidStr = values.get("oid");
	  commStr = values.get("communityString");
	  int tempVersion = Integer.parseInt(values.get("version"));
	  if(tempVersion == 1){
		  snmpVersion = SnmpConstants.version1;
	  }
	  if(tempVersion == 2){
		  snmpVersion = SnmpConstants.version2c;
	  }
	  if(tempVersion == 3){
		  snmpVersion = SnmpConstants.version3;
	  }
	  portNum =  "161";
  }
  
  public void doSnmpwalk() throws IOException  {
    Address targetAddress = GenericAddress.parse("udp:"+ targetAddr + "/" + portNum);
    TransportMapping transport = new DefaultUdpTransportMapping();
    Snmp snmp = new Snmp(transport);
    transport.listen();
  
    // setting up target
    CommunityTarget target = new CommunityTarget();
    target.setCommunity(new OctetString(commStr));
    target.setAddress(targetAddress);
    target.setRetries(3);
    target.setTimeout(1000 * 3);
    target.setVersion(snmpVersion);
  
    OID oid = null;
    try{
      oid = new OID(oidStr);
    }
    catch(RuntimeException ex){
      System.out.println("OID is not specified correctly.");
      System.exit(1);
    }
  
    TreeUtils treeUtils = new TreeUtils(snmp, new DefaultPDUFactory());      
    List<TreeEvent> events = treeUtils.getSubtree(target, oid);
    if(events == null || events.size() == 0){
      System.out.println("No result returned.");
      System.exit(1);
    }
    
    // Get snmpwalk result.
    for (TreeEvent event : events) {
      if(event != null){
        if (event.isError()) {
            System.err.println("oid [" + oid + "] " + event.getErrorMessage());
          }
            
        VariableBinding[] varBindings = event.getVariableBindings();
        if(varBindings == null || varBindings.length == 0){
          System.out.println("No result returned.");
        }
        for (VariableBinding varBinding : varBindings) {
          results.add(varBinding.getVariable().toString());
        }
      }
    }
    
    //print results
    for(String value: results){
    	System.out.println(value);
    }
    snmp.close();
  }
  
  public boolean isInResults(String value){
	  return results.contains(value);
  }
    
  
  private void checkAndSetArgs(String[] args){
    if(args.length < 2){
      System.err.println(usage);
      System.exit(1);
    }//if
    
    for (int i=0; i<args.length; i++){
      if("-c".equals(args[i])){
        commStr = args[++i]; 
      }
      else if ("-v".equals(args[i])){
        if(Integer.parseInt(args[++i]) == 1){
          snmpVersion = SnmpConstants.version1;
        }
        else {
          snmpVersion = SnmpConstants.version2c;
        }
      }//if
      else if ("-p".equals(args[i])){
        portNum = args[++i];
      }
      else{
        targetAddr = args[i++];
        oidStr = args[i];
      }
    }//for
    if(targetAddr == null || oidStr == null){
      System.err.println(usage);
      System.exit(1);
    }
  }
  
}//class