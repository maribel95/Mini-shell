/*
Miembros del grupo ( Panas Operativos ):

Odilo Fortes Domínguez
Sandu Bizu
María Isabel Crespí Valero
*/

// Librerías
#include <stdio.h>      /* para printf en depurarión */
#include <string.h>    /* para funciones de strings  */
#include <stdlib.h>    /* Funciones malloc(), free(), y valor NULL */
#include <fcntl.h>     /* Modos de apertura de función open()*/
#include <sys/stat.h>  /* Permisos función open() */
#include <sys/types.h> /* Definiciones de tipos de datos como size_t*/
#include <unistd.h>    /* Funciones read(), write(), close()*/
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
// Constantes
#define COMMAND_LINE_SIZE 1024
#define PROMPT '$'
#define ARGS_SIZE 64
#define N_JOBS 64
#define _POSIX_C_SOURCE 200112L
#define minishell "./my_shell"
// Constantes de los colorines
#define NEGRO_F "\x1b[40m"
#define BLANCO_T   "\x1b[37m"
#define MAGENTA_T  "\x1b[35m"
#define CYAN_T     "\x1b[36m"
// Funciones
char *read_line(char *line); 
void imprimir_prompt(); 
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
//Funciones auxiliares
char *crearNuevoDirectorio(int boolFree,char **args,char *nuevoDirectorio);
void imprimir_prompt();
int is_background();
int jobs_list_add(pid_t pid, char status, char *cmd);
int jobs_list_find(pid_t pid);
int  jobs_list_remove(int pos);
int is_output_redirection (char **args);
//Manejadores
void reaper(int signum);
void ctrlc(int signum);
void ctrlz(int signum);
//Estructuras
struct info_process { 
   pid_t pid; //Pid del proceso
   //Posibles status:
   // ‘N’(ninguno), ’E’(Ejecutándose), ‘D’(Detenido), ‘F’(Finalizado)
   char status;
   char cmd[COMMAND_LINE_SIZE]; // línea de comando
};
//Variables globales
static struct info_process jobs_list[N_JOBS];
static int n_pids = 1;
static int esSegundoPlano;

/* ---------------------------------------             MINI SHELL           -------------------------------------------------*/

//Código
int main(){
    // Inicializamos los valores del proceso en primer plano
    jobs_list[0].pid = 0;
    jobs_list[0].status = 'N';

    char line[COMMAND_LINE_SIZE];
    // Preparamos el main para que reciba las señales
    // a sus respectivos manejadores
    signal(SIGCHLD, reaper); 
    signal(SIGINT, ctrlc);
    signal(SIGTSTP, ctrlz);
    //Bucle infinito, 1 se considera verdadero 
    while(1){
        if(read_line(line) && strlen(line) > 0){
            execute_line(line);
        }   
    }
    return 0;
}

//Primera función que lo que hace es imprimir el promt
// y leer una línea de consola
char *read_line(char *line){
    char *linea = "";
    //Imprimimos el promt por pantalla
    imprimir_prompt();
    // Vaciamos el flujo de salida por si acaso
    fflush(stdout);
    // Leemos y comprobamos que el flujo de entrada
    // no es NULL
    if( fgets(line,COMMAND_LINE_SIZE,stdin) ){
        linea = line;

    }else{        
        printf("\n");        
        if (feof(stdin)) { //feof(stdin)!=0 significa que se ha activado el indicador EOF
          exit(0);
        }else{
            linea = NULL;
        } 
    }
    return linea;
}

// Función auxiliar que imprime el promt
void imprimir_prompt(){
    
    //Imprimimos el user y el home de color
    // cyan y magenta respectivamente
    printf( CYAN_T NEGRO_F"%s" BLANCO_T NEGRO_F":" 
    MAGENTA_T NEGRO_F"~%s" BLANCO_T NEGRO_F
    "%c"" ",getenv("USER"),getenv("HOME"),PROMPT);

}

// Función para ejecutar la línea introducida por teclado
int execute_line(char *line){    
    // Crearemos una variable interna llamada cmdsinSalto que contenga el comando
    // introducido por teclado.
    char cmdsinSalto[COMMAND_LINE_SIZE];
    //Inicializamos el parámetro args
    char *args[ARGS_SIZE];
    // Copiamos el comando original antes de ser troceado
    strcpy(cmdsinSalto,line);
    // En caso de que tenga un salto de línea, se lo quitamos
    strtok(cmdsinSalto,"\n");
    
    // Ahora rellenamos args con los cachitos de la instrucción introducida
    parse_args(args, line);
    //Miramos si el comando introducido es en segundo plano o no
    esSegundoPlano = is_background(args);
    // Si check internal devuelve -1 es que no se ha
    // introducido un comando interno
    if(check_internal(args) == -1){
        // Creamos un proceso hijo
        pid_t pid = fork();
        // Si el pid es 0, significa que estamos mirando el proceso hijo.

        if (pid == 0) { // HIJO
            signal(SIGINT,SIG_IGN); // Ignora la señal
            signal(SIGCHLD,SIG_DFL); // Hace la acción predeterminada
            signal(SIGTSTP,SIG_IGN); // Ignora la señal

            //Llamamos a esta función para saber si hay que hacer redireccionamiento de flujo
            is_output_redirection(args); 

            //Ejecutamos execvp
            execvp(args[0],args);
            //A partir de aquí, si retorna es que algo ha ido mal
            fprintf(stderr,"%s: no se encontró la orden\n",args[0]); // Comunicamos el error
            
            exit(-1); // Acaba el proceso hijo

        } else if (pid >0) { // PADRE
            // Si el proceso es en segundo plano, añadiremos un nuevo trabajo a la lista
            if(esSegundoPlano == 1){
                //sleep(1);
                jobs_list_add(pid,'E',cmdsinSalto);

            }else{ //Si es en primer plano
                //Inicializamos los campos para el proceso en Foreground
                jobs_list[0].pid = pid; // Asignamos el pid en la primera fila de job_list
                jobs_list[0].status = 'E'; //Cambiamos el estado del proceso a Ejecución
                memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd)); //Limpiamos por si acaso
                strcpy(jobs_list[0].cmd,cmdsinSalto); // Le asignamos su comando correspondiente                
                //Esperamos a que acabe cualquier hijo
                //Mientras el proceso siga activo, el padre quedará esperando
                while(jobs_list[0].pid != 0){
                    pause();
                }
            }
        } else { //pid < 0 error
            perror("fork");
            exit(EXIT_FAILURE);
        }     
    }
    return 1;
}

// Método auxiliar para saber si un proceso es en segundo plano o no
int is_background(char **args){

    //Haremos una búsqueda del carácter & o hasta llegar a NULL
    int i = 0; //iterador
    int esForeground = 0; // 0 = false y 1 = true

    while( args[i] != NULL ){
        //Si hemos encontrado un &, entonces es un proceso background
        if( *args[i] == '&'){
            esForeground = 1; // true porque hemos encontrado un &, es un proceso en background
            args[i] = NULL; // Substituimos & por NULL
        }
        i++;        
    }
    return esForeground;
}

// Función que controlará la lista de trabajos.
int jobs_list_add(pid_t pid, char status, char *cmd){
    // Miramos que no hayamos llegado al número máximo de trabajos permitidos
    if( n_pids < N_JOBS ){
        //Rellenamos los campos del proceso en segundo plano
        jobs_list[n_pids].pid = pid;
        jobs_list[n_pids].status = status;
        strcpy(jobs_list[n_pids].cmd,cmd);
        // Imprimimos
        printf("[%d] %d\t%c\t%s\n",n_pids,jobs_list[n_pids].pid,jobs_list[n_pids].status,jobs_list[n_pids].cmd);
        // Y ya colocamos el índice de la lista una posición más abajo
        n_pids++;

    }else{
        printf("No se ha podido añadir el trabajo ya que la lista está llena.\n");
        return 0;
    }

    return 1;

}

// Método para encontrar el índice de la lista de arrays según un PID
int jobs_list_find(pid_t pid){
    int indice = 0; // índice del trabajo en segundo plano
    int encontrado = 0; // 0 = false, 1 = true

    // Hacemos un for para hacer el recorrido y encontrarlo
    while(encontrado == 0 && indice < n_pids){
        indice++;
        if(jobs_list[indice].pid == pid){
            encontrado = 1;
        }
    }
    
    return indice;
}

// Método para eliminar trabajos
int  jobs_list_remove(int pos){
    int ultimaPos = n_pids - 1; //Queremos el último elemento de la lista
    //Solo hay un elemento o coincide el último con el que estamos buscando
    if(n_pids == 2 || (n_pids -1 ) == pos){ 
        // Simplemente limpiamos
        jobs_list[pos].pid = 0;
        jobs_list[pos].status = 'N';
        memset(jobs_list[pos].cmd,0,sizeof(jobs_list[pos].cmd));
        n_pids--;
        return n_pids;

    }else if (n_pids > 2 && n_pids != pos){ //Si hay varios elementos
        // Sobreescribiremos la posición que queremos eliminar con
        // el último trabajo de la lista
        // Rellenamos los campos correspondientes
        jobs_list[pos].pid = jobs_list[ultimaPos].pid;
        jobs_list[pos].status = jobs_list[ultimaPos].status;
        strcpy(jobs_list[pos].cmd,jobs_list[ultimaPos].cmd);
        n_pids--;
        return n_pids;
    }else{ //Si no se cumple ninguno de estos casos, dará error.
        perror("Error");
        return 0;
    }

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
// es una función interna o no
int check_internal(char **args){
    int valorRetorno = 1;
    if(args[0] == NULL){
        valorRetorno = -2;
        return valorRetorno; 
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
        // un comando interno
        valorRetorno = -1;
    }
    return valorRetorno;
}


// Función para hacer el cambio de directorio
int internal_cd(char **args){
   
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
        //Miramos si es un directorio 'compuesto' (que vengas varios directorios delimitados por /).
        if(strchr(directorioAuxiliar,'/') && !strchr(directorioAuxiliar,'.')){
            // Declaramos variables auxiliares
            char *argsAux[ARGS_SIZE];
            char *dirAux[3];
            iterador = 0;
            //Queremos separar todos los directorios introducidos
            argsAux[iterador] = strtok(directorioAuxiliar,"/");
            // Y lo vamos haciendo con todos
            while(argsAux[iterador]){
                iterador++;
                argsAux[iterador] = strtok(NULL,"/");                
            }
            // Y ahora trataremos todos esos directorios de manera individual 
            for(int i = 0; i < iterador;i++){
                // Si hay un espacio en blanco, significa que los delimitadores son las comillas
                if(strchr(argsAux[i],' ')){
                    dirAux[1] = strtok(argsAux[i]," "); 
                    
                    dirAux[2] = strtok(NULL," ");            
                    
                    nuevoDirectorio = crearNuevoDirectorio(boolFree,dirAux,nuevoDirectorio);
                //Si hay un \, entonces trataremos este caso en particular
                }else if(strchr(argsAux[i],'\\')){
                    dirAux[1] = argsAux[i]; 
                    nuevoDirectorio = crearNuevoDirectorio(boolFree,dirAux,nuevoDirectorio);
                // Este es el caso donde no se han introducido ni comillas ni \ para el directorio
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
        // Este sería el caso simple donde solo han introducido el directorio y ya esta( sin / que separe otros directorios).
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
    }
    return 1;
}

//Función auxiliar para el cd_avanzado
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

// Función para exportar variables
int internal_export(char **args){
    //Variables auxiliar 
    char *aux = "USER";
    //Hacemos el primer troceo
    char *nombre = strtok(args[1],"=");

    // Si nombre está vacío o no es igual a USER, entonces es erroneo
    if( !nombre || strcmp(nombre,aux) != 0){
        fprintf(stderr,"Error de sintaxis. Uso: export Nombre=Valor\n");
        return -1;
    }else{
        char *valor = strtok(NULL," \t\r\n");
        //Si la segunda parte es correcta
        if( valor ){
            int err = setenv("USER",valor,1);

            //Pequeña comprobación de que la función del sistema no da error
            if (err == -1) {          
                perror("Error");
                return -1; 
            }

        }else{
            fprintf(stderr,"Error de sintaxis. Uso: export Nombre=Valor\n");
            return -1;
    }
    
    }
    return 1;

}

// Función para ejecutar un archivo línea a línea
int internal_source(char **args){

    //Variables
    char *nombreFichero = args[1];
    char *linea = malloc(COMMAND_LINE_SIZE);
    //Comprobamos de que se haya introducio un nombre de 
    // algún fichero
    if(args[1]){
        //Abrimos el fichero en modo lectura
        FILE *f = fopen(nombreFichero,"r");
        //Comprobamos que la llamada a fopen no haya salido mal
        if(!f){ // f == NULL
            perror("fopen");
            return -1;
        }
        //Leemos linea a linea el contenido del fichero
        while(fgets( linea,COMMAND_LINE_SIZE,f) && strlen(linea) > 1){
            fflush(f); // Forzamos vaciado del buffer por si acaso
            execute_line(linea); // Y ejecutamos la siguiente linea
        }
        //Cerramos fichero
        fclose(f);
            
       
    // Este sería el caso en el que args[1] = NULL
    // y por lo tanto, no se ha introducido nada
    // después de source.
    }else{
        fprintf(stderr,"Error de sintaxis. Uso: source <nombre_fichero>\n");
        return -1;
    }

    return 1;
}
//Imprimiremos la lista de trabajos en foreground
int internal_jobs(char **args){
    int indiceJobs = 1; //índice de los trabajos
    //Imprimimos todos los trabajos de la lista que no estén en foreground
    while( indiceJobs < n_pids ){
        printf("[%d] %d\t%c\t%s\n",indiceJobs,jobs_list[indiceJobs].pid,jobs_list[indiceJobs].status,jobs_list[indiceJobs].cmd);
        indiceJobs++;
    }
    
    return 1;
}

// Función para pasar de segundo a primer plano
int internal_fg(char **args){

    if(args[1]){
        int pos = *args[1] - '0'; //Sacamos la posición de trabajo
        if (pos >= n_pids || pos == 0){ // Excepción: el trabajo que se ha introducido es incorrecto
            fprintf(stderr,"No existe ese trabajo\n");
            return -1;
        }else if (pos > 0){ // Si se ha introducido un índice de trabajo correcto
            if(jobs_list[pos].status == 'D'){ //Miramos si el estado es detenido
                kill(jobs_list[pos].pid,SIGCONT);
            }
            // Pasamos el proceso a foreground
            jobs_list[0].pid = jobs_list[pos].pid;
            jobs_list[0].status = 'E';
            strcpy( jobs_list[0].cmd, strtok( jobs_list[ pos ].cmd, "&" ) );
            printf("%s\n",jobs_list[0].cmd);
            //Ahora borramos el proceso del background
            jobs_list_remove(pos);

            while(jobs_list[0].pid != 0){
                pause();
            }
        }
    }else{
        fprintf(stderr,"No existe ese trabajo\n");
        return -1;
    }
    
    return 1;
}

// Función para pasar un proceso detenido a ejecución
int internal_bg(char **args){

    if(args[1]){
        int pos = *args[1] - '0'; //Sacamos la posición de trabajo
        if (pos >= n_pids || pos == 0){ // Miramos que se haya introducido un índice de trabajo razonable
            fprintf(stderr,"No existe ese trabajo\n");
            return -1;
        }else{
            if(jobs_list[pos].status == 'E'){
                fprintf(stderr,"El trabajo ya está en segundo plano\n");
                return -1;
            }else{
                jobs_list[pos].status = 'E';
                strcat(jobs_list[pos].cmd," &"); // Le añadimos el & al final del comando
                kill(jobs_list[pos].pid,SIGCONT);
                // Imprimimos el trabajo
                printf("[%d] %d\t%c\t%s\n",pos, jobs_list[pos].pid, jobs_list[pos].status, jobs_list[pos].cmd );
            }
        }
    }else{
        fprintf(stderr,"No existe ese trabajo\n");
        return -1;       
    }
    return 1;
}

//Manejador para cuando un hijo muera y así evitar procesos zombies
void reaper(int signum){
    signal(SIGCHLD,reaper); // Por si acaso
    int estado;
    pid_t ended;
    int indiceJobs; 
    
    while( (ended = waitpid( -1 , &estado , WNOHANG )) > 0){
        // Si el proceso acabado es el foreground
        if( ended == jobs_list[0].pid ){ 
            jobs_list[0].pid = 0; //Reestablecemos el PID a cero
            jobs_list[0].status = 'F'; //El estado del proceso es Finalizado
            memset(jobs_list[0].cmd, '\0', sizeof(jobs_list[0].cmd)); //Limpiamos comando
        
        }else{ //Si es es background
            indiceJobs = jobs_list_find(ended); //Encontramos el índice del proceso finalizado
            fprintf(stderr,"Terminado PID %d (%s) en jobs_list[%d] con status %d\n",jobs_list[indiceJobs].pid,jobs_list[indiceJobs].cmd,indiceJobs,jobs_list[indiceJobs].status);
            jobs_list_remove(indiceJobs); //Borramos dicho proceso
            
        }
    
    }

}

// Manejador para cuando llegue una señal de que se ha pulsado Ctrl+C
void ctrlc(int signum){
    
    signal(SIGINT,ctrlc); // Por si acaso
	//Si hay un proceso en foreground
	if(jobs_list[0].pid > 0){
		//Si el proceso en foreground NO es el mini shell
		if(strcmp(jobs_list[0].cmd, minishell) != 0){
			kill(jobs_list[0].pid, SIGTERM);
		}
	} 

    fflush(stdout); //Forzamos vaciado de salida del buffer.

}

//Manejador que recogerá cuando se pulse Ctrl + Z
void ctrlz(int signum){
    signal(SIGTSTP,ctrlz); // Por si acaso
	//Si hay un proceso en foreground
	if(jobs_list[0].pid > 0){
		//Si el proceso en foreground NO es el mini shell
		if(strcmp(jobs_list[0].cmd, minishell) != 0){

			kill(jobs_list[0].pid, SIGSTOP);
            jobs_list[0].status = 'D';
            jobs_list_add(jobs_list[0].pid,jobs_list[0].status,jobs_list[0].cmd);

            //Limpiamos todos los campos del proceso en Foreground
            jobs_list[0].pid = 0;
            jobs_list[0].status = 'N';
            memset(jobs_list[0].cmd,0,sizeof(jobs_list[0].cmd)); //Limpiamos el comando

			
		}
	}
    fflush(stdout); //Forzamos vaciado de salida del buffer.
}

// Función para saber si hay redirección de flujo
int is_output_redirection (char **args){
    
    int indice = 0;
    int esRedireccion = 0; //0 = false y 1 = true
    while(esRedireccion == 0 && args[indice] != NULL){
        if(*args[indice] == '>'){
            esRedireccion = 1;
        }
        indice++;

    }
    
    if(args[indice]){
        args[indice -1] = NULL; // Eliminamos el '>' 
        int fd = open(args[indice],  O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
        //Vamos comprobando si cada función nos da error
        if( fd == -1){
            perror("Open");
            exit(EXIT_FAILURE);
        }
        // Hacemos el duplicado con el flujo de salida estándard
        if(dup2(fd,1) == -1){
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        // Cerramos el fichero
        if(close(fd) == -1){
            perror("close");
            exit(EXIT_FAILURE);
        }
        
    }else{
        esRedireccion = 0; //Porque no hay fichero
    }

    return esRedireccion;
}
/* ---------------------------------------             FIN             --------------------------------------------------------*/