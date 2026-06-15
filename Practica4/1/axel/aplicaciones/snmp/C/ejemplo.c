/*gcc ejemplo.c -o ejemplo -lnetsnmp*/
#include <stdio.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>

int main()
{
    netsnmp_session session, *ss;
    netsnmp_pdu *pdu;
    netsnmp_pdu *response;
    oid anOID[MAX_OID_LEN];
    size_t anOID_len;
    int status;
    char *peername = "localhost"; // Dirección IP o nombre del dispositivo SNMP
    char *community = "public"; // Comunidad SNMP

    init_snmp("snmpapp"); // Inicializar la biblioteca SNMP

    snmp_sess_init(&session); // Inicializar la estructura de sesión SNMP
    //session.version = SNMP_VERSION_2c;
    session.version = SNMP_VERSION_1;
    session.peername = peername;
    session.community = (unsigned char *)community;
    session.community_len = strlen(community);

    SOCK_STARTUP; // Inicializar el socket

    ss = snmp_open(&session); // Abrir la sesión SNMP

    if (!ss)
    {
        snmp_perror("snmp_open");
        SOCK_CLEANUP;
        exit(1);
    }

    // Convertir el OID de texto a una representación numérica
    anOID_len = MAX_OID_LEN;
    if (!snmp_parse_oid("1.3.6.1.2.1.1.4.0", anOID, &anOID_len))
    {
        snmp_perror("snmp_parse_oid");
        snmp_close(ss);
        SOCK_CLEANUP;
        exit(1);
    }

    // Crear una nueva PDU SNMP de tipo GET
    pdu = snmp_pdu_create(SNMP_MSG_GET);
    snmp_add_null_var(pdu, anOID, anOID_len); // Agregar el OID a la PDU

    status = snmp_synch_response(ss, pdu, &response); // Enviar la solicitud SNMP

    if (status == STAT_SUCCESS && response->errstat == SNMP_ERR_NOERROR)
    {
        // Imprimir el valor obtenido
        printf("Valor del OID 1.3.6.1.2.1.1.4.0: %s\n", response->variables->val.string);
    }
    else
    {
        if (status == STAT_SUCCESS)
        {
            fprintf(stderr, "Error en la respuesta SNMP: %s\n", snmp_errstring(response->errstat));
        }
        else
        {
            snmp_sess_perror("snmp_synch_response", ss);
        }
    }

    if (response)
    {
        snmp_free_pdu(response); // Liberar la PDU de respuesta
    }

    snmp_close(ss); // Cerrar la sesión SNMP
    SOCK_CLEANUP; // Limpiar el socket

    return 0;
}
