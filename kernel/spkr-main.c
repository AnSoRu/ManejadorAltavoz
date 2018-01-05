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
#define FIFO_SIZE = getpagesize(void);
#define kfifoDefined 0
#endif

dev_t midispo; 
struct cdev dev;
struct class *clase;
struct device *dispoDevice;
	
//unsigned int major;
//unsigned int minor;
//unsigned int count;

unsigned int major;
unsigned int minor=0;
unsigned int count=1;

//void cdev_init(struct cdev *dev, struct file_operations *fops);
//int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
//void cdev_del(struct cdev *dev);
//struct class * class_create (struct module *owner, const char *name); 
//struct device * device_create(struct class *class, struct device *parent, dev_t devt, void *drvdata, const char *fmt);
//class_destroy(struct class * clase);
//device_destroy(struct class * class, dev_t devt);

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
static int finish(void){
	
//liberación de los números major y minor asociados
 unregister_chrdev_region(midispo, count);

 //Asimismo, se debe añadir el código de eliminación del dispositivo (cdev_del) en la rutina de descarga del módulo. La operación de baja del dispositivo
 cdev_del(&dev);
 
 //En la rutina de descarga del módulo habrá que invocar a device_destroy y a class_destroy para dar de baja el dispositivo y la clase en sysfs.
 device_destroy(clase,midispo);
 
//A la hora de descargar el módulo habrá que hacer la operación complementaria (class_destroy(struct class * clase)).
class_destroy(clase);

return 0;

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
	int count_write=0;
	int count_read=0;
	
	//Hay que asegurarse de que en cada momento sólo está abierto una vez en modo escritura el fichero. 
	if (filp->f_mode & FMODE_WRITE){
		count_write++;
		printk(KERN_INFO "operacion de apertura en modo escritura\n");

		//Si se produce una solicitud de apertura en modo escritura estando ya abierto en ese mismo modo. Se retornará el error -EBUSY. 
		if (count_write>1){
			return -EBUSY;
		}
	}
	else{ //filp->f_mode & FMODE_READ
		//Sin embargo, no habrá ninguna limitación con las aperturas en modo lectura.
		count_read++;
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
/*Se trata de escribir en el buffer los sonidos en espera para que se vayan reproduciendo*/
static ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
			printk(KERN_INFO "Iniciado operacion de escritura\n");
  //buf es un puntero de la zona de memoria del usuario
  //no se puede acceder desde el kernel directamente a esta zona pues esta paginado y las direcciones no concuerdan
  //copy_from_user -> para copiar datos desde el espacio de memoria del usuario
  //put_user(datum,ptr) -> escribe 'datum' en el espacio de usuario
 //el tamaño del buffer interno se recibe como parametro 'buffer_size'
 //si no se especifica sera igual al tamaño de una pagina PAGE_SIZE
 if(!kfifoDefined){

 }else{

 
 }
/*Primero hay que comprobar si hay espacio suficiente para escribir */
 /*Si hay espacio*/

 //Si no hay espacio -> ¿se bloquea el proceso?


 printk(KERN_INFO "Finalizada operacion de escritura\n");
 return count;
}

static struct file_operations fops = {
     .owner = THIS_MODULE,
     .open = open,
     .release = release,
     .write = write
};

static int init(void){
   //Reserva del major
   alloc_chrdev_region(&midispo,minor,count,nombre_dispo);
   major = MAJOR(midispo);
   cdev_init(&dev,&fops);
   cdev_add(&dev,midispo,count);
   clase = class_create(THIS_MODULE,clase_dispo);
   device_create(clase,NULL,midispo,NULL,nombre_dispo);
 return 0;
}

