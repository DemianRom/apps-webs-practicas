/*gcc get.c -o get -lnetsnmp*/
#include <stdio.h>
#include <stdlib.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
//#include "/home/axel/net-snmp-5.8/include/net-snmp/net-snmp-config.h"
//#include "/home/axel/net-snmp-5.8/include/net-snmp/net-snmp-includes.h"
int main() {
    // Inicializar la biblioteca SNMP
    init_snmp("snmpapp");

    // Variables para almacenar la información de la solicitud y respuesta SNMP
    netsnmp_session session, *ss;
    netsnmp_pdu *pdu, *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    netsnmp_variable_list *vars;
    char* community = "public";

    // Configurar la sesión SNMP
    snmp_sess_init(&session);
    //session.peername = strdup("172.24.52.213"); // Dirección IP del agente SNMP
    session.peername = "172.24.52.213"; // Dirección IP del agente SNMP
    //session.version = SNMP_VERSION_2c;
    session.version = SNMP_VERSION_1;
    session.community = (unsigned char *)community; // Comunidad SNMP
    //session.community = "public";
    session.community_len = strlen(community);
    // Abrir la sesion SNMP
    SOCK_STARTUP;
    ss = snmp_open(&session);
    if (!ss) {
        snmp_sess_perror("snmpapp", &session);
        SOCK_CLEANUP;
        exit(1);
    }

    // Crear la solicitud SNMP GET
    //pdu = snmp_pdu_create(SNMP_MSG_GET);
    anOID_len = MAX_OID_LEN;
    if (!snmp_parse_oid("SNMPv2-MIB::sysUpTime.0", anOID, &anOID_len)) {
//    if (!snmp_parse_oid(".1.3.6.1.2.1.1.4.0", anOID, &anOID_len)) {
        snmp_perror("snmpapp");
        snmp_close(ss);
        SOCK_CLEANUP;
        exit(1);
    }
    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, anOID, anOID_len);

    // Enviar la solicitud SNMP
    if (snmp_synch_response(ss, pdu, &response) == STAT_SUCCESS) {
        if (response->errstat == SNMP_ERR_NOERROR) {
            // Obtener y mostrar el valor de la variable SNMP
            vars = response->variables;
            if (vars->type == ASN_TIMETICKS) {
                printf("Valor de sysUpTime.0: %ld\n", *(vars->val.integer));
            } 
/*            if (vars->type == ASN_OCTET_STR) {
                printf("Valor del OID: %s\n", vars->val.string);
            } */

       // printf("Valor del OID: %s\n",response->variables->val.string);
        } else {
            fprintf(stderr, "Error en la respuesta SNMP: %s\n", snmp_errstring(response->errstat));
        }
    } else {
        snmp_sess_perror("snmpapp", ss);
    }

    // Liberar la respuesta SNMP y cerrar la sesión SNMP
    if (response)
        snmp_free_pdu(response);
    snmp_close(ss);
    SOCK_CLEANUP;

    return 0;
}
