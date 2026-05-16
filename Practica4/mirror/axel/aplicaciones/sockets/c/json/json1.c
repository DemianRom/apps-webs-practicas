/**Biblioteca jansson**/
/*  gcc json.c -l jansson -o json  ***/

#include <stdio.h>
#include <jansson.h>

json_t* create_record()
{
    const char* nombre = "Pedro";
    const char* ciudad = "CDMX";
    int edad = 30;

    json_t* root = json_object();
    json_t* jtemp = NULL;

    jtemp = json_string(nombre);
    json_object_set_new(root, "nombre", jtemp);
    jtemp = json_string(ciudad);
    json_object_set_new(root, "ciudad", jtemp);
    jtemp = json_integer(edad);
    json_object_set_new(root, "edad", jtemp);

    return root;
}

void print_record(json_t* root)
{
    json_t* jtemp = NULL;
    char* nombre = NULL;
    char* ciudad = NULL;
    int edad;

    jtemp = json_object_get(root, "nombre");
    nombre = (char*) json_string_value(jtemp);
    jtemp = json_object_get(root, "ciudad");
    ciudad = (char*) json_string_value(jtemp);
    jtemp = json_object_get(root, "edad");
    edad = (int) json_integer_value(jtemp);

    printf("%s %s %d\n", nombre, ciudad, edad);
}

void print_json(json_t* root)
{
    char* temp = json_dumps(root, JSON_INDENT(4));
    printf("%s\n", temp);
    free(temp);
}

void remove_record(json_t* root)
{
    json_decref(root);
}

int main(int argc, char const *argv[])
{
    json_t* root = create_record();
    print_record(root);
    print_json(root);
    //////////////////////////
    char* cadena_json = json_dumps(root, JSON_INDENT(4));
    printf("Cadena lista para ser enviada:\n%s\n", cadena_json);
    json_t* nuevo_json = json_loads(cadena_json,0,NULL);
    printf("Json nuevamente cargado.. obteniendo informacion..\n");
    char* nombre1 = NULL;
    char* ciudad1 = NULL;
    int edad1;
    json_t* jtemp1 = NULL;       
    jtemp1 = json_object_get(nuevo_json, "nombre");
    nombre1 = (char*) json_string_value(jtemp1);
    jtemp1 = json_object_get(nuevo_json, "ciudad");
    ciudad1 = (char*) json_string_value(jtemp1);
    jtemp1 = json_object_get(nuevo_json, "edad");
    edad1 = (int) json_integer_value(jtemp1);
    printf("%s %s %d\n", nombre1, ciudad1, edad1);
    print_json(nuevo_json);  
    ////////////////////////////////
    remove_record(root);
    return 0;
}
