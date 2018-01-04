#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/i8253.h>
#include <linux/spinlock_types.h>
//#include <sys/uio.h>

raw_spinlock_t bloqueo;
unsigned long flags;
uint32_t DivisorFreq;
uint8_t value;
uint8_t value1;

/*******************************************************
 FASE 1: FRECUENCIA SPEAKER, SPEAKER ON Y SPEAKER OFF
*******************************************************/

/*Funcion para configurar los temporizadores
1)Seleccionar modo de operacion
2)Establecer el valor del contador del temporizador
Primero la parte baja del contador y luego la alta
Cuando se programa el chip 8253/8254 es necesario realizar dos escrituras al registro de datos para enviar primero la parte baja del contador y, 
a continuación, la parte alta. Por tanto, es necesario crear una sección crítica en el acceso a ese hardware*/

//Dado que este hardware se accede tanto desde el contexto de  una llamada como desde el de una interrupción, 
//hay que usar las funciones que protegen también contra las interrupciones: raw_spin_lock_irqsave y raw_spin_unlock_irqrestore.

//controlar acceso: spinlock (denominado i8253_lock de tipo raw spinlock)
void set_spkr_frequency(unsigned int frequency) {
	printk(KERN_INFO "spkr set frequency: %d\n", frequency);
	//divisores de frecuencia de una entrada de reloj que oscila a 1,193182 MHZ
        DivisorFreq = (uint32_t)(PIT_TICK_RATE / frequency);
	flags = 0;
	raw_spin_lock_irqsave(&bloqueo,flags);

	//Configurar uno de estos temporizadores, en primer lugar, es necesario seleccionar su modo de operación escribiendo en su registro de control (puerto 0x43)
	//el valor correspondiente (en nuestro caso, hay que seleccionar el timer 2 y especificar un modo de operación 3) 
	 outb(0xb6, 0x43);
	
	//A continuación, hay que escribir en su registro de datos (puerto 0x42) el valor del contador del temporizador. 
	//Al tratarse de un puerto de 8 bits y dado que el contador puede tener hasta 16 bits, se pueden necesitar dos escrituras para configurar el contador.
	//Referencia: http://wiki.osdev.org/PC_Speaker
	
 	outb((uint8_t) (DivisorFreq), 0x42);
 	outb((uint8_t) (DivisorFreq >> 8), 0x42);
	raw_spin_unlock_irqrestore(&bloqueo,flags);
}

/*Funcion para activar el altavoz:
Conectar el reloj del sistema a la entrada del temporizador 2. Esto es-> escribir un 1 en el bit 1 del puerto 0x61
Pero para que llegue a la entrada del altavoz es necesario escribir un 1 en el bit 0 del puerto 0x61
En definitiva hay que escribir un 1 en los dos bits de menor peso del puerto 0x61*/ 
   
void spkr_on(void) {
	printk(KERN_INFO "spkr ON\n");
	//en el registro 0x61 hay que escribir un 1 en el bit 1 y un 1 en el bit 0 del puerto 0x61
	raw_spin_lock_irqsave(&bloqueo,flags);
        value = inb(0x61);
	outb(value | 3, 0x61);
	raw_spin_unlock_irqrestore(&bloqueo,flags);

}

//Simplemente es escribir un 0 en cualquiera de los dos bits de menor peso
void spkr_off(void) {
	printk(KERN_INFO "spkr OFF\n");
	raw_spin_lock_irqsave(&bloqueo,flags);
	value1 = inb(0x61) & 0xFC;
 	outb(value1,0x61);
	raw_spin_unlock_irqrestore(&bloqueo,flags);
}


