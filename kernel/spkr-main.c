#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/kfifo.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

extern void set_spkr_frequency(unsigned int frequency);
extern void spkr_on(void);
extern void spkr_off(void);

/**************************************
FASE 2:
***************************************/

#define nombre_dispo "intspkr"
#define clase_dispo "speaker"

#ifndef FIFO_SIZE
#define FIFO_SIZE 4096
#endif

dev_t midispo; 
struct cdev dev;
struct class *clase;
struct device *dispoDevice;
struct mutex m_open;
struct mutex dis_variable;
struct mutex mutex_write;
wait_queue_head_t lista_bloq;
static struct kfifo cola;
//static struct kfifo_rec_ptr_1 cola;
struct timer_list t_list;

#if 0
#define DYNAMIC
#endif

//#ifdef DYNAMIC
//static DECLARE_KFIFO_PTR(cola,unsigned char);
//#else
//static DEFINE_KFIFO(cola,unsigned char,FIFO_SIZE);
//#endif

unsigned int major;
unsigned int minor=0;
unsigned int count=1;
unsigned int count_write=0;
unsigned int count_read=0;
unsigned int buffer_size = PAGE_SIZE;

//Variables auxiliares para la funcion write
unsigned int contador = 0;
unsigned int desplazamiento = 0;
//unsigned int copied;

//unsigned char sound[4];
unsigned int frecuencia;
unsigned int duracion;

//module_param(buffer_size,int,S_IRUGO);


//unsigned int buffer_threshold = PAGE_SIZE;
unsigned int buffer_threshold = PAGE_SIZE;
static int flag = 0;
static char message[256]={0};
static char aux[20]={0};
static short size_of_message;
int ret;
int ret_k_init;
int altavoz_encendido = 0;
int silenciar_spkr = 0;
//void cdev_init(struct cdev *dev, struct file_operations *fops);
//int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
//void cdev_del(struct cdev *dev);
//struct class * class_create (struct module *owner, const char *name); 
//struct device * device_create(struct class *class, struct device *parent, dev_t devt, void *drvdata, const char *fmt);
//void class_destroy(struct class * clase);
//void device_destroy(struct class * class, dev_t devt);

void reproducir(unsigned long data);

//Funcion auxiliar para reproducir el sonido
void reproducir(unsigned long data){
   printk(KERN_INFO "Reproduciendo\n");
   char sound[4];   
   //Nos aseguramos que hay al menos un sonido
   while(kfifo_len(&cola) >= 4){
      printk(KERN_INFO "Hay %d\n",kfifo_len(&cola));
      kfifo_out(&cola,sound,4);
      printk("El sonido recibido es %c %c %c %c",sound[0],sound[1],sound[2],sound[3]);     
      printk("El sonido recibido es %c-%c-%c-%c",sound[0],sound[1],sound[2],sound[3]);
      
      frecuencia = ((int)sound[1] << 8) | sound[0];
      duracion = ((int)sound[3] << 8) | sound[2];
      
      printk(KERN_INFO "Frecuencia %d\n",frecuencia);
      printk(KERN_INFO "Duracion %d\n",duracion);

      //t_list.data = contador;
      //t_list.expires = jiffies + msecs_to_jiffies(duracion);
      //Caso en el que no sea un silencio
      if(frecuencia!=0){
        set_spkr_frequency(frecuencia);
        if(!silenciar_spkr){
	  //printk(KERN_INFO "Speaker ON\n");
          printk(KERN_INFO "Añadiendo timer-sonido\n");
          spkr_on();
          //add_timer(&t_list);
          mod_timer(&t_list,jiffies + msecs_to_jiffies(duracion));
        }
      }else{
       //Caso en el que sea un silencio
       //printk(KERN_INFO "Speaker OFF\n");
       printk(KERN_INFO "La frecuencia es 0\n");
       silenciar_spkr = 0;
       spkr_off();
       printk(KERN_INFO "Añadiendo timer-silencio\n");
       //add_timer(&t_list);
       mod_timer(&t_list,jiffies + msecs_to_jiffies(duracion));
      }
      if((kfifo_avail(&cola)) >= buffer_threshold){
        wake_up_interruptible(&lista_bloq);
        printk(KERN_INFO "Desbloqueado proceso escritor\n");
      }
   }//En la cola no hay mas de 4 bytes
   /*if(kfifo_len(&cola)<4 && !kfifo_is_empty(&cola)){
     printk(KERN_INFO "No hay un sonido completo\n");
     silenciar_spkr = 1;
     //del_timer(&t_list);
     spkr_off();
   }*/
  if(((kfifo_len(&cola)<4) && (altavoz_encendido==1))){
        altavoz_encendido = 0;
  	spkr_off();
  }
  printk(KERN_INFO "Saliendo de Reproducir\n");
  //del_timer(&t_list);
}




//Añada a la rutina de terminación del módulo la llamada a la función unregister_chrdev_region para realizar la liberación correspondiente.
static void __exit finish(void){
	printk(KERN_INFO "Finalizando\n");
	
    //liberación de los números major y minor asociados
	unregister_chrdev_region(midispo, count);
	//Asimismo, se debe añadir el código de eliminación del dispositivo (cdev_del) en la rutina de descarga del módulo. La operación de baja del dispositivo
	cdev_del(&dev);
	//En la rutina de descarga del módulo habrá que invocar a device_destroy y a class_destroy para dar de baja el dispositivo y la clase en sysfs.
	device_destroy(clase,midispo);
	//A la hora de descargar el módulo habrá que hacer la operación complementaria (class_destroy(struct class * clase)).
	class_destroy(clase);
	spkr_off();
	//Eliminar la cola kfifo
	kfifo_free(&cola);
        printk(KERN_INFO "Saliendo de finish\n");
}

/**************************************
FASE 3: FUNCION APERTURA
***************************************/
//recibe dos parametros: 
//Un puntero al inodo de fichero (struct inode *) y un puntero a la estructura file (struct file *) que representa un descriptor de un fichero abierto
static int open(struct inode *inode, struct file *filp) {
		printk(KERN_INFO "operacion de apertura\n");
		
//i_cdev (struct cdev *i_cdev) y es un puntero a la estructura cdev que se usó al crear el dispositivo.
//f_mode: almacena el modo de apertura del fichero. Para determinar el modo de apertura, se puede comparar ese campo con las constantes FMODE_READ y FMODE_WRITE.
  
	//Hay que asegurarse de que en cada momento sólo está abierto una vez en modo escritura el fichero. 
	if (filp->f_mode & FMODE_WRITE){
		mutex_lock_interruptible(&m_open);
		count_write++;
		printk(KERN_INFO "operacion de apertura en modo escritura\n");
		printk(KERN_INFO "contador de escritura: %d\n",count_write );

		//Si se produce una solicitud de apertura en modo escritura estando ya abierto en ese mismo modo. Se retornará el error -EBUSY. 
		if (count_write>1){
                        mutex_unlock(&m_open); //Antes de retornar de la funcion hay que liberar el mutex
			return -EBUSY;
		}
		mutex_unlock(&m_open);
	}
	else{ //filp->f_mode & FMODE_READ
		//Sin embargo, no habrá ninguna limitación con las aperturas en modo lectura.
                mutex_lock_interruptible(&m_open);
		count_read++;
                mutex_unlock(&m_open);
		printk(KERN_INFO "operacion de apertura en modo lectura\n");
	}
  printk(KERN_INFO "Saliendo de open\n");
  return 0;		
}

static int release(struct inode *inode, struct file *filp) {			
       printk(KERN_INFO "operacion de cierre\n");
       if(filp->f_mode & FMODE_WRITE){
             mutex_lock_interruptible(&m_open);
             count_write--;
             printk(KERN_INFO "operacion de cierre en modo escritura\n");
             printk(KERN_INFO "contador de escritura: %d\n",count_write);
       mutex_unlock(&m_open);
       }else{
            mutex_lock_interruptible(&m_open);
            count_read--;
            mutex_unlock(&m_open);
            printk(KERN_INFO "operacion de cierre en modo lectura\n");
       }
  printk(KERN_INFO "Saliendo de Release\n");
  return 0;
}

/*****************************************************
FASE 4: OPERACION DE ESCRITURA
*****************************************************/

/*Se comporta como un productor de sonidos*/
/*Utilizar la estructura kfifo*/
/*Se trata de escribir en el buffer (en nuestro caso la cola) los sonidos en espera para que se vayan reproduciendo*/
//ES UN PRODUCTOR DE SONIDOS
static ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
 printk(KERN_INFO "Iniciado operacion de escritura\n");
 
 unsigned int copied;
 //Fijarse en el siguiente código 
 //https://github.com/torvalds/linux/blob/master/samples/kfifo/inttype-example.c
 sprintf(message,"%s(%zu)",buf,count);
 size_of_message = strlen(message);
  
 printk(KERN_INFO "Recibidos %zu caracteres desde el usuario",count);
 printk(KERN_INFO "Recibidos %s\n",message);

 printk(KERN_INFO "El desplazamiento es %d\n",desplazamiento);
 
 printk(KERN_INFO "El buf es %s\n",buf);

 contador = count;

 printk(KERN_INFO "El valor del contador al inicio del write es %d\n",contador);
   
  while(count>0){
      if(wait_event_interruptible(lista_bloq,kfifo_len(&cola)<=FIFO_SIZE)!=0){
         return -ERESTARTSYS;
      }
      if((count + kfifo_len(&cola))%4!=0){
       if(mutex_lock_interruptible(&mutex_write))
           return -ERESTARTSYS;
       if(kfifo_from_user(&cola,buf,count,&copied)!=0){
           return -EFAULT;
         }
       mutex_unlock(&mutex_write);
       break;
      }else{
       if(mutex_lock_interruptible(&mutex_write))
           return -EFAULT;
       if(kfifo_from_user(&cola,buf,count,&copied)!=0){
           return -EFAULT;
       }
       mutex_unlock(&mutex_write);
       count-=copied;
       buf+=copied;
       if(!altavoz_encendido){
	altavoz_encendido = 1;
        reproducir(0);
       }
     }
  }

   printk(KERN_INFO "Finalizada operacion de escritura\n");
   printk(KERN_INFO "Se han escrito %d bytes\n",copied);
   printk(KERN_INFO "Saliendo de operacion write\n");
   return contador;
}

//if LINUX_VERSION_CODE <= KERNEL_VERSION(3,0,0)
//retorna el número de versión del núcleo para el que se está compilando el módulo 
//LINUX_VERSION_CODE
//KERNEL_VERSION(a,b,c): genera el entero que representa la versión correspondiente a los valores pasados como parámetros.

static int fsync(struct file *filp, loff_t start, loff_t end, int datasync) {
	//   int wait_event_interruptible(wait_queue_head_t cola, int condicion);
		wait_event_interruptible(lista_bloq, kfifo_is_empty(&cola));
 return 0;
}
int spkr_fsync(struct file *filp, int datasync);


static struct file_operations fops = {
     .owner = THIS_MODULE,
     .open = open,
     .release = release,
     .write = write,
	 .fsync = fsync
};


static int __init init(void){
   printk(KERN_INFO "Inicializando\n");
   //set_spkr_frequency(50);
   //spkr_on();
   //Se debe incluir dentro de la rutina de iniciación del módulo una llamada a alloc_chrdev_region para reservar un dispositivo llamado spkr, 
   //cuyo major lo seleccionará el sistema, mientras que el minor corresponderá al valor recibido como parámetro. 
   alloc_chrdev_region(&midispo,minor,count,"spkr");
   //Reserva del major
   //Un dispositivo en Linux queda identificado por una pareja de números: el major identifica al manejador, y el minor identifica al dispositivo concreto entre los que gestiona ese manejador.
   major = MAJOR(midispo);
   	//iniciar esa estructura de dato
   cdev_init(&dev,&fops);
   	//Después de iniciar la estructura que representa al dispositivo, hay que asociarla con los identificadores de dispositivo reservados previamente
   cdev_add(&dev,midispo,count);
   //Para dar de alta al dispositivo en sysfs, en la iniciación del módulo se usarán las funciones class_create y device_create. 
   //Después de ejecutar esta llamada, aparecerá la entrada correspondiente en sysfs (/sys/class/speaker/).
   clase = class_create(THIS_MODULE,clase_dispo);
   //Después de crear la clase, hay que dar de alta el dispositivo de esa clase. Para ello, se usa la función device_create:
   device_create(clase,NULL,midispo,NULL,nombre_dispo);
   printk(KERN_INFO "El major asignado es: %d\n", major);
   printk(KERN_INFO "El minor asignado es: %d\n", minor);
   printk(KERN_INFO "El tamaño del buffer es: %d\n", buffer_size);
   printk(KERN_INFO "El umbral limite es %d\n",buffer_threshold);
   //iniciamos mutex
   mutex_init(&m_open);
   mutex_init(&dis_variable);
   mutex_init(&mutex_write);
   //Inicializar la cola kfifo
   ret_k_init = kfifo_alloc(&cola,buffer_size,GFP_KERNEL);
   if(ret_k_init){
     printk(KERN_ERR "Error en init al hacer kfifo_alloc\n");
     return ret;
   }else{
     printk(KERN_INFO "Cola interna inicializada\n");
   }

   setup_timer(&t_list,reproducir,0);
   //INIT_KFIFO(cola);
   //Inicializar la wait queue
   init_waitqueue_head(&lista_bloq);
   printk(KERN_INFO "Saliendo de Init\n");
 return 0;
}

module_param(minor,int,S_IRUGO);
module_param(buffer_size,int,S_IRUGO);
module_param(buffer_threshold,int,S_IRUGO);
//module_param(buffer_threshold,int,S_IRUGO);
module_init(init);
module_exit(finish);
