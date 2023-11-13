/*
Miembros del grupo ( Panas Operativos ):

Odilo Fortes Domínguez
Sandu Bizu
María Isabel Crespí Valero
*/

//Librerías
#include <stdio.h>      /* para printf en depurarión */
#include <string.h>    /* para funciones de strings  */
#include <stdlib.h>    /* Funciones malloc(), free(), y valor NULL */

// Constantes
#define COMMAND_LINE_SIZE 1024
#define PROMPT '$'
#define ARGS_SIZE 64
// Constantes de los colorines
#define NEGRO_F "\x1b[40m"
#define BLANCO_T   "\x1b[37m"
#define MAGENTA_T  "\x1b[35m"
#define CYAN_T     "\x1b[36m"

//Funciones
char *read_line(char *line); 
void imprimir_prompt(); // Función auxiliar
int parse_args(char **args, char *line);
int check_internal(char **args); 
int execute_line(char *line);
int check_internal(char **args); 
// Funciones de las siguientes misiones
int internal_cd(char **args); 
int internal_export(char **args); 
int internal_source(char **args); 
int internal_jobs(char **args);
int internal_fg(char **args); 
int internal_bg(char **args); 


int main(){

    char line[COMMAND_LINE_SIZE];
    //Bucle infinito, 1 se considera verdadero
    while(1){
     if(read_line(line) && strlen(line)>0){
         execute_line(line);
     }   
    }

    return 0;
}

//Primera función que lo que hace es imprimir el promt
// y leer una línea de consola
char *read_line(char *line){
    //Imprimimos el promt por pantalla
    imprimir_prompt();
    // Vaciamos el flujo de salida por si acaso
    fflush(stdout);
    // Leemos y comprobamos que el flujo de entrada
    // no es NULL
    if( fgets(line,COMMAND_LINE_SIZE,stdin) ){
        return line;
    }else{
        // Ha habido algún error y retorna NULL
        return NULL;
    }
    
}

// Función auxiliar que imprime el promt
void imprimir_prompt(){
    //Imprimimos el user y el home de color
    // cyan y magenta respectivamente
    printf( CYAN_T NEGRO_F"%s" BLANCO_T NEGRO_F":" 
    MAGENTA_T NEGRO_F"~%s" BLANCO_T NEGRO_F
    "%c"" ",getenv("USER"),getenv("HOME"),PROMPT);

}

// Llama a las funciones de los tokens y de
// comprobar si el primer token es una función
// interna.
int execute_line(char *line){
    char *args[ARGS_SIZE];
    parse_args(args, line);
    check_internal(args);
    return 1;

}
// Función que trocea el comando  en tokens
int parse_args(char **args, char *line){
    // Declaración de variables
    const char delimitadores[5] = " \t\n\r";
    int iterador=0;
    char *token;
    // Leemos el primer token y lo guardamos en args
    token = strtok(line, delimitadores);
    args [iterador] = token; 
    int seguir = 1;
   
    //Leemos todos los demás tokens
    while( token && seguir == 1){

        if(args[ iterador ][0] == '#'){
            seguir = 0;
        }
        // Los imprimimos
        printf( "[parse_args()->token %d: %s]\n ",iterador, args[iterador]);
        // El iterador aumenta para poder guardar el nuevo token
        // en el siguiente espacio del array
        
        iterador++;

        
        
        //Leemos el token
        token = strtok(NULL, delimitadores);
        //Y lo guardamos en args

        args [iterador] = token; 

        
    }
    //Excepciones
    if(iterador == 0){
            args[0] = NULL;
            printf( "[parse_args()->token %d: %s]\n ",iterador ,args[iterador ]);
        }else{
            iterador--;
    
            if(args[iterador][0] == '#'){
                args[iterador] = NULL;
                printf( "[parse_args()->token %d corregido: %s]\n ",iterador, args[iterador]);
            }else{
                printf( "[parse_args()->token %d: %s]\n ",iterador +1 ,args[iterador + 1]);
            }
        }
    
    return iterador;
   
   
   
}

// Función que comprueba si el primer token 
// es una función interna
int check_internal(char **args){

    int success = 1;
    if(args[0] == NULL){
        args[0] = "excepción";
    }

    if(strcmp(args[0],"cd") == 0){
        internal_cd(args);

    }else if(strcmp(args[0],"export") == 0){
        internal_export(args);

    }else if(strcmp(args[0],"source") == 0){
        internal_source(args);

    }else if(strcmp(args[0],"jobs") == 0){
        internal_jobs(args);

    }else if(strcmp(args[0],"fg") == 0){
        internal_fg(args);

    }else if(strcmp(args[0],"bg") == 0){
        internal_bg(args);


    }else if(strcmp(args[0],"exit") == 0){
        exit(0);

    }else{
        // Este else quiere decir que no se ha introducido
        // unc comando interno
        success = -1;
    }
    return success;
}

// En esta primera misión, las funciones de los comandos internos
// tan solo imprimiran lo que hacen cada uno.

int internal_cd(char **args){
    printf("Es una orden utilizada para cambiar el directorio de trabajo.\n");
    return 1;
}

int internal_export(char **args){
    printf("Se utiliza para exportar variables, cuando nosotros ejecutamos"
    "un programa todas las variables del entorno exportadas son heredadas"
    "a los procesos hijos y ellos pueden acceder a ellas.\n");
    return 1;

}

int internal_source(char **args){
    printf("Este comando hace que el proceso o comando se ejecute"
    "sin crear ningún proceso hijo, de forma que los cambios efectuados"
    "en las variables de entorno y demás, se mantengan al finalizar el"
    "archivo.\n");
    return 1;
}

int internal_jobs(char **args){
    printf("se utiliza para listar procesos que estés ejecutando"
    "en segundo plano o en primer plano.\n");
    return 1;
}

int internal_fg(char **args){
    printf("traerá a primer plano un trabajo que está ejecutándose en segundo plano.\n");
    return 1;
}

int internal_bg(char **args){
    printf("Esta secuencia nos permite poner un proceso en segundo plano"
    "y desasociarlo de la shell en la que estamos trabajando.\n");
    return 1;
}


