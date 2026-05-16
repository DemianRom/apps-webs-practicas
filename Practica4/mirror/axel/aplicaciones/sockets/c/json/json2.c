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

void add_2array_to_json( json_t* obj, const char* name, const int*
marr, size_t dim1, size_t dim2 )
{
    size_t i, j;
    json_t* jarr1 = json_array();

    for( i=0; i<dim1; ++i ) {
        json_t* jarr2 = json_array();

        for( j=0; j<dim2; ++j ) {
            int val = marr[ i*dim2 + j ];
            json_t* jval = json_integer( val );
            json_array_append_new( jarr2, jval );
        }
        json_array_append_new( jarr1, jarr2 );
    }
    json_object_set_new( obj, name, jarr1 );
    return;
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
    
    json_t* dato;
    char* s;
    int arr1[2][3] = { {1,2,3}, {4,5,6} };
    dato = json_object();
    add_2array_to_json( dato, "arreglo1", &arr1[0][0], 2, 3 );
    s = json_dumps( dato, 0 );
    puts( s );
    //printf("cadena obtenida: \n%s\n",s);
    json_t* dato_nuevo = json_loads(s,0,NULL);
    printf("\nJson cargado nuevamente");
    print_json(dato_nuevo);
     /***************************/
     json_t *dato_tmp = json_object_get(dato_nuevo,"arreglo1");
      //int off = 0;
     if(json_is_array(dato_tmp)){
        const uint lenght = json_array_size(dato_tmp);
        printf("tam filas: %d\n",lenght);
        for(uint ii=0; ii<lenght; ii++){
            json_t *objeto = json_array_get(dato_tmp,ii);
           if(json_is_array(objeto)){
            const uint lenght2 = json_array_size(objeto);
            printf("tam columnas: %d\n",lenght2);
             for(uint jj=0; jj<lenght2;jj++){
                // off = (ii*lenght2)+jj;
                 json_t *numero = json_array_get(objeto,jj);
                 int x = (int)json_integer_value(numero);
                 printf("numero[%d][%d]: %d\n ",ii,jj,x);
             }//for
           }//if 
        }//for
     }//if



    /*******************************/
//    int f = (int)json_array_size(dato); 
//    printf("filas: %d\n",f);
    free( s );
    json_decref( dato );
    json_decref(dato_nuevo);
    
    
    return 0;
}
