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
#include <asm/uaccess.h>



/**************************************
FASE 2:
***************************************/

//Reserva y liberación del dispositivo
//Un dispositivo en Linux queda identificado por una pareja de números: 
//el major, que identifica al manejador, y el minor, que identifica al dispositivo concreto entre los que gestiona ese manejador.


//Se debe incluir dentro de la rutina de iniciación del módulo una llamada a alloc_chrdev_region para reservar un dispositivo llamado spkr, 
//cuyo major lo seleccionará el sistema, mientras que el minor corresponderá al valor recibido como parámetro. 

#define dispo "spkr"
#define nombre_dispo "speaker"
dev_t midispo; 
	
int mayor, minor, count;
major=MAJOR(midispo);
minor=0;
count=1;

void cdev_init(struct cdev *dev, struct file_operations *fops);
int cdev_add(struct cdev *dev, dev_t num, unsigned int count);
void cdev_del(struct cdev *dev);
struct class * class_create (struct module *owner, const char *name); 
struct device * device_create(struct class *class, struct device *parent, dev_t devt, void *drvdata, const char *fmt);
class_destroy(struct class * clase);
device_destroy(struct class * class, dev_t devt);

static struct file_operations fops = {
        .owner =    THIS_MODULE,
        .open =     open,
        .release =  release,
        .write =    write
};

static int init(void){
	//Reserva de mayor y minor
	int alloc_chrdev_region(&midispo, minor, count, dispo);
	//Una vez reservado el número de dispositivo, hay que incluir en la función de carga del módulo la iniciación (cdev_init) y alta (cdev_add) del dispositivo.
	//iniciar esa estructura de datos
	cdev_init(dev,fops);
	//Después de iniciar la estructura que representa al dispositivo, hay que asociarla con los identificadores de dispositivo reservados previamente
	cdev_add(dev,midispo,count);
	
	
//Para dar de alta al dispositivo en sysfs, en la iniciación del módulo se usarán las funciones class_create y device_create. 

//class_create: Como primer parámetro se especificaría THIS_MODULE y en el segundo el nombre que se le quiere dar a esta clase de dispositivos (en este caso, speaker). 
//Después de ejecutar esta llamada, aparecerá la entrada correspondiente en sysfs (/sys/class/speaker/).
class_create(THIS_MODULE,nombre_dispo);

//Después de crear la clase, hay que dar de alta el dispositivo de esa clase. Para ello, se usa la función device_create:
device_create(class_create, NULL, midispo, NULL,dispo)

}

//Añada a la rutina de terminación del módulo la llamada a la función unregister_chrdev_region para realizar la liberación correspondiente.
static int finish(void){
	
//liberación de los números major y minor asociados
void unregister_chrdev_region(midispo, count);

 //Asimismo, se debe añadir el código de eliminación del dispositivo (cdev_del) en la rutina de descarga del módulo. La operación de baja del dispositivo
 cdev(dev);
 
 //En la rutina de descarga del módulo habrá que invocar a device_destroy y a class_destroy para dar de baja el dispositivo y la clase en sysfs.
 device_destroy(class_create,midispo);
 
//A la hora de descargar el módulo habrá que hacer la operación complementaria (class_destroy(struct class * clase)).
class_destroy(class_create);

}


//operaciones de apertura, cierre y escritura.

static int open(struct inode *inode, struct file *filp) {
		printk(KERN_INFO "operatcion de apertura\n");
}

static int release(struct inode *inode, struct file *filp) {
			printk(KERN_INFO "operatcion de cierre\n");

}

static ssize_t write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
			printk(KERN_INFO "operatcion de escritura\n");

}





