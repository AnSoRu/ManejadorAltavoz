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
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");

extern void set_spkr_frequency(unsigned int frequency);
extern void spkr_on(void);
extern void spkr_off(void);

/**************************************
FASE 2:
***************************************/

//Reserva y liberación del dispositivo
//Un dispositivo en Linux queda identificado por una pareja de números: 
//el major, que identifica al manejador, y el minor, que identifica al dispositivo concreto entre los que gestiona ese manejador.


//Se debe incluir dentro de la rutina de iniciación del módulo una llamada a alloc_chrdev_region para reservar un dispositivo llamado spkr, 
//cuyo major lo seleccionará el sistema, mientras que el minor corresponderá al valor recibido como parámetro. 

#define nombre_dispo "intspkr"
#define clase_dispo "speaker"



#ifndef FIFO_SIZE
// int size = getconf(PAGE_SIZE);
#define FIFO_SIZE 4096
#endif

dev_t midispo; 
struct cdev dev;
struct class *clase;
struct device *dispoDevice;
struct mutex m_open;
wait_queue_head_t lista_bloq;

#if 0
#define DYNAMIC
#endif

#ifdef DYNAMIC
static DECLARE_KFIFO_PTR(cola,unsigned char);
#else
static DEFINE_KFIFO(cola,unsigned char,FIFO_SIZE);
#endif

	
//unsigned int major;
//unsigned int minor;
//unsigned int count;

unsigned int major;
unsigned int minor=0;
unsigned int count=1;
unsigned int count_write=0;
unsigned int count_read=0;
static int flag = 0;
int ret;
unsigned int copied;

//void cdev_init(struct cdev *dev, struct file_operations *fops);
//int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
//void cdev_del(struct cdev *dev);
//struct class * class_create (struct module *owner, const char *name); 
//struct device * device_create(struct class *class, struct device *parent, dev_t devt, void *drvdata, const char *fmt);
//void class_destroy(struct class * clase);
//void device_destroy(struct class * class, dev_t devt);

/*static struct file_operations fops = {
        .owner =    THIS_MODULE,
        .open =     open,
        .release =  release,
        .write =    write
};*/

//static int init(void){
	//Reserva de mayor y minor

//	int alloc_chrdev_region(&midispo, minor, count, dispo);

	//Una vez reservado el número de dispositivo, hay que incluir en la función de carga del módulo la iniciación (cdev_init) y alta (cdev_add) del dispositivo.

	//iniciar esa estructura de datos
//	cdev_init(&dev,&fops);

	//Después de iniciar la estructura que representa al dispositivo, hay que asociarla con los identificadores de dispositivo reservados previamente
//	cdev_add(&dev,midispo,count);
	
	
//Para dar de alta al dispositivo en sysfs, en la iniciación del módulo se usarán las funciones class_create y device_create. 

//class_create: Como primer parámetro se especificaría THIS_MODULE y en el segundo el nombre que se le quiere dar a esta clase de dispositivos (en este caso, speaker). 
//Después de ejecutar esta llamada, aparecerá la entrada correspondiente en sysfs (/sys/class/speaker/).

//class_create(THIS_MODULE,clase_dispo);

//Después de crear la clase, hay que dar de alta el dispositivo de esa clase. Para ello, se usa la función device_create:

//device_create(class_create, NULL, midispo, NULL,dispo)

//Inicializar una cola de tipo kfifo donde se iran almacenando los sonidos


//}

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

}


//operaciones de apertura, cierre y escritura.


/**************************************
FASE 3: FUNCION APERTURA
***************************************/
//recibe dos parametros: 
//Un puntero al inodo de fichero (struct inode *) y un puntero a la estructura file (struct file *) que representa un descriptor de un fichero abierto
static int open(struct inode *inode, struct file *filp) {
		printk(KERN_INFO "operacion de apertura\n");
		
//i_cdev (struct cdev *i_cdev) y es un puntero a la estructura cdev que se usó al crear el dispositivo.
//f_mode: almacena el modo de apertura del fichero. Para determinar el modo de apertura, se puede comparar ese campo con las constantes FMODE_READ y FMODE_WRITE.
	//Hay que definir estos dos contadores fuera de esta función pues si no cada vez que se llame a open se vuelven a inicializar y por tanto las comprobaciones posteriores no van a tener el efecto deseado.
        //count_write=0;
	//count_read=0;
        
	
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
  return 0;		
}

static int release(struct inode *inode, struct file *filp) {
			printk(KERN_INFO "operacion de cierre\n");
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
 
 //Fijarse en el siguiente código 
 //https://github.com/torvalds/linux/blob/master/samples/kfifo/inttype-example.c


 //buf es un puntero de la zona de memoria del usuario
  //no se puede acceder desde el kernel directamente a esta zona pues esta paginado y las direcciones no concuerdan
  //copy_from_user -> para copiar datos desde el espacio de memoria del usuario
  //put_user(datum,ptr) -> escribe 'datum' en el espacio de usuario
 //el tamaño del buffer interno se recibe como parametro 'buffer_size'
 //si no se especifica sera igual al tamaño de una pagina PAGE_SIZE
 // int ret;
 // unsigned int copied; 
  if(mutex_lock_interruptible(&m_open))
      return -1;
  /*Primero hay que comprobar si hay espacio suficiente para escribir */
   /*Si no hay espacio*/
   if(kfifo_initialized(&cola)){
     if(kfifo_is_empty(&cola)){
          //no hay sonidos en la cola
         ret = kfifo_from_user(&cola,buf,count,&copied);
     }else{
         //ver si esta llena o si queda espacio
         while((kfifo_is_full(&cola)) || (kfifo_avail(&cola)<count)){
                //Se bloquea
                //Pero antes de bloquearme tengo que liberar el mutex
                mutex_unlock(&m_open);
                flag = 1;
                wait_event_interruptible(lista_bloq,flag != 1); // => esto hay que usarlo en el read
                //wake_up_interruptible(&lista_bloq);
         }
     }
   }else{
     //No está inicializada
     printk(KERN_INFO "Error en write. La cola kfifo no esta inicializada\n");
   }
   mutex_unlock(&m_open);

   printk(KERN_INFO "Finalizada operacion de escritura\n");
   return ret ? ret : copied;
}

static struct file_operations fops = {
     .owner = THIS_MODULE,
     .open = open,
     .release = release,
     .write = write
};

static int __init init(void){
   printk(KERN_INFO "Inicializando\n");
   set_spkr_frequency(50);
   //spkr_on();
   alloc_chrdev_region(&midispo,minor,count,nombre_dispo);
   //Reserva del major
   major = MAJOR(midispo);
   cdev_init(&dev,&fops);
   cdev_add(&dev,midispo,count);
   clase = class_create(THIS_MODULE,clase_dispo);
   device_create(clase,NULL,midispo,NULL,nombre_dispo);
   printk(KERN_INFO "El major asignado es: %d\n", major);
   printk(KERN_INFO "El minor asignado es: %d\n", minor);
   //iniciamos mutex
   mutex_init(&m_open);
   //Inicializar la cola kfifo
   INIT_KFIFO(cola);
   //Inicializar la wait queue
   init_waitqueue_head(&lista_bloq);
 return 0;
}

module_init(init);
module_exit(finish);
