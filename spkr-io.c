#include<stdio.h>


//FASE 1

//Funcion para configurar los temporizadores
//1)Seleccionar modo de operacion
//2)Establecer el valor del contador del temporizador
//Primero la parte baja del contador y luego la alta
void set_spkr_frequency(){

}

//Funcion para activar el altavoz
//1)Conectar el reloj del sistema a la entrada del temporizador 2
//esto es-> escribir un 1 en el bit 1 del puerto 0x61
//Pero para que llegue a la entrada del altavoz es necesario escribir un 1 en el bit 
//0 del puerto 0x61
//En definitiva hay que escribir un 1 en los dos bits de menor peso del puerto 0x61
//Nota: este puerto 0x61 es utilizado por otros manejadores por lo que hay que asegurarse
/*una vez escrito los dos bits, que los demas quedan inalterados*/
void spkr_on(){


}

//Simplemente es escribir un 0 en cualquiera de los dos bits de menor peso
void spkr_off(){


}


int main(int argc, char **argv){
 if(argc > 1){
    printf("%s\n",argv[1]);
 }
 printf("HolaMundo\n");
 return 0;
}
