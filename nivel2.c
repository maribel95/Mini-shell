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
#include <fcntl.h>     /* Modos de apertura de función open()*/
#include <sys/stat.h>  /* Permisos función open() */
#include <sys/types.h> /* Definiciones de tipos de datos como size_t*/
#include <unistd.h>    /* Funciones read(), write(), close()*/


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

char *crearNuevoDirectorio(int boolFree,char **args,char *nuevoDirectorio);

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
    while( token && seguir == 1 ){
        //Como solo podemos poner un comentario
        // haremos break en el primer que encontremos
        if(args[ iterador ][0] == '#'){
            seguir = 0;
        }
        
        // El iterador aumenta para poder guardar el nuevo token
        // en el siguiente espacio del array
        iterador++;
        //Leemos el token
        token = strtok(NULL, delimitadores);
        //Y lo guardamos en args

        args [iterador] = token; 
        
    }

        if(iterador == 0){
            args[0] = NULL;
            
        }else{
            iterador--;
    
            if(args[iterador][0] == '#'){
                args[iterador] = NULL;
                
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



int internal_cd(char **args){
    //String donde guardaremos el nombre completo del directorio.
    char cwd[1024];

    //Imprime el directorio actual y sino da error.
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
       printf("El directorio actual es: %s\n", cwd);
    } else {
       perror("chdir");
       return -1;
    }
    //Comprobamos que nos venga algo en args[1]
    if(args[1]){
        
        //Esta variable nos servirá para saber si después tendremos que hacer un free
        //Creamos un puntero auxiliar al nuevo directorio
        int boolFree = 0;
        char *nuevoDirectorio = args[1];
        char *directorioAuxiliar = malloc(COMMAND_LINE_SIZE );
        strcpy(directorioAuxiliar,args[1]);
        // Miramos
        int iterador = 2;
        //Unificamos todo el nombre del directorio en un solo string.
        while(args[iterador]){ //args[iterador] != NULL
            strcat(directorioAuxiliar," ");
            strcat(directorioAuxiliar,args[iterador]);
            iterador++;

        }
        
        if(strchr(directorioAuxiliar,'/') && !strchr(directorioAuxiliar,'.')){
            char *argsAux[ARGS_SIZE];
            char *dirAux[3];
            iterador = 0;

            argsAux[iterador] = strtok(directorioAuxiliar,"/");
            
            while(argsAux[iterador]){
                iterador++;
                argsAux[iterador] = strtok(NULL,"/");
                
            }
            
            for(int i = 0; i < iterador;i++){

                if(strchr(argsAux[i],' ')){
                    dirAux[1] = strtok(argsAux[i]," "); 
                    
                    dirAux[2] = strtok(NULL," ");            
                    
                    nuevoDirectorio = crearNuevoDirectorio(boolFree,dirAux,nuevoDirectorio);
                }else if(strchr(argsAux[i],'\\')){
                    dirAux[1] = argsAux[i]; 
                    nuevoDirectorio = crearNuevoDirectorio(boolFree,dirAux,nuevoDirectorio);
                }else{
                    nuevoDirectorio = argsAux[i]; 
                }

                
                //Ahora hacemos el cambio de directorio
                if (chdir(nuevoDirectorio) != 0) {
                    perror("chdir");
                    return -1;
                }
                //Aquí comprobamos si hicimos un malloc antes.
                if(boolFree == 1){
                    free(nuevoDirectorio);
                    boolFree = 0;
                }

            }
            free(directorioAuxiliar);
            printf("Se ha cambiado correctamente el directorio.\n");
            
            

        }else{
            
            nuevoDirectorio = crearNuevoDirectorio(boolFree,args,nuevoDirectorio);
            //Ahora hacemos el cambio de directorio
            if (chdir(nuevoDirectorio) != 0) {
                perror("chdir");
                return -1;
            }
            //Aquí comprobamos si hicimos un malloc antes.
            if(boolFree == 1){
                free(nuevoDirectorio);
            }

        }

        
        
        //fin de cd_avanzado

    
    }
    return 1;
}

char *crearNuevoDirectorio(int boolFree,char **args,char *nuevoDirectorio){
    const char ch1 = '\\' ;
    const char ch2 = '\"';
    const char ch3 ='\'' ;
    //Los delimitadores
    const char delim[4] = "\\\'\"";

    if(strchr(args[1],ch2) || strchr(args[1],ch3)){
        boolFree = 1;
        // Si las palabras que nos llegan tienes comillas
        // hay que "limpiarlas" para dejar la palabra
        // sin nada más.
        args[1] = strtok(args[1], delim);
        args[2] = strtok(args[2], delim);
        // El nuevo directorio apuntará a una región de memoria reservada
        // para el nombre del nuevo directorio bien construido
        nuevoDirectorio = malloc(sizeof(args[1]) + sizeof(args[2]));
        // A continuación, construiremos un String pero con el
        // nombre real del directorio al cual nos queremos mover.
        strcpy(nuevoDirectorio, args[1]);
        strcat(nuevoDirectorio, " ");
        strcat(nuevoDirectorio, args[2]);
        // Ya tenemos el nuevo nombre de directorio.

        //Aquí seguimos la misma lógica que arriba, pero esta vez 
        // si nos encontramos una barra
    }else if(strchr(args[1],ch1)){
        boolFree = 1;
        args[1] = strtok(args[1], delim);
        args[2] = strtok(NULL, delim);
        nuevoDirectorio = malloc(sizeof(args[1]) + sizeof(args[2]));
        strcpy(nuevoDirectorio, args[1]);
        strcat(nuevoDirectorio, " ");
        strcat(nuevoDirectorio, args[2]);
    }
        return nuevoDirectorio;
}


int internal_export(char **args){
    //Variables auxiliar 
    char *aux = "USER";
    //Hacemos el primer troceo
    char *nombre = strtok(args[1],"=");

    // Si nombre está vacío o no es igual a USER, entonces es erroneo
    if( !nombre || strcmp(nombre,aux) != 0){
        printf("Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }else{
        //Comprobamos la segunda parte del parámetro
        printf("[internal_export()→ nombre: %s]\n",nombre);
        char *valor = strtok(NULL," \t\r\n");
        printf("[internal_export()→ valor: %s]\n",valor);
        //Si la segunda parte es correcta
        if( valor ){
            printf("[internal_export()→ antiguo valor para USER: %s]\n",getenv("USER"));
            int err = setenv("USER",valor,1);

            //Pequeña comprobación de que la función del sistema no da error
            if (err == -1) {          
                perror("Error");
                return -1; 
            }

            printf("[internal_export()→ nuevo valor para USER: %s]\n",getenv("USER"));
        }else{
            printf("Error de sintaxis. Uso: export Nombre=Valor\n");
            return -1;
    }
    
    }
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
