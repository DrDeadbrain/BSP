 
/* #include <linux/config.h> */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cdev.h>
#include <linux/errno.h>  /* error codes */
#include <linux/fcntl.h>  /* O_ACCMODE */
#include <linux/fs.h>     /* everything... */
#include <linux/kernel.h> /* printk() */
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
//#include <linux/shed/signal.h> //TODO fix include
#include <linux/slab.h>  /* kmalloc() */
#include <linux/types.h> /* size_t */

//#include <asm/system.h>		/* cli(), *_flags */
/* #include <asm/uaccess.h>	/\* copy_*_user *\/ */
#include <linux/uaccess.h> /* copy_*_user */

#include "ceasar.h" /* local definitions */

#define NUMBER_OF_READERS 1 //Means 1 in this config
#define NUMBER_OF_WRITERS 1
/*
 * Our parameters which can be set at load time.
 */

struct buffer_pipe {
        wait_queue_head_t inq, outq; /* read and write queues */ //? Outq for Writers.. Inq for readers .. ?!
        char *buffer, *end;                                      /* begin of buf, end of buf */
        int buffersize;                                          /* used in pointer arithmetic */
        char *rp, *wp;                                           /* where to read, where to write */
        int nreaders, nwriters;                                  /* number of openings for r/w */
        struct fasync_struct *async_queue;                       /* asynchronous readers */
        struct semaphore sem; /* mutual exclusion semaphore */   //? Also das nur ein process zurzeit auf das Device zugreifen kann
        struct cdev cdev; /* Char device structure */            //? Das Device welches das file (die Pipe) verwaltet oder benutzt oder was
};

static struct buffer_pipe *buffer_p;

static atomic_t reading_slots = ATOMIC_INIT(NUMBER_OF_READERS);
static atomic_t writing_slots = ATOMIC_INIT(NUMBER_OF_WRITERS);

int ceasar_major = CEASAR_MAJOR;
int ceasar_minor = 0;                //? Start minor num for Devices
int ceasar_nr_devs = CEASAR_NR_DEVS; /* number of bare ceasar devices */


module_param(ceasar_major, int, S_IRUGO);
module_param(ceasar_minor, int, S_IRUGO);
module_param(ceasar_nr_devs, int, S_IRUGO);


MODULE_AUTHOR("Linus Kurz, Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

struct ceasar_dev *ceasar_devices; /* allocated in ceasar_init_module */

#define KRYPTO_KEY 1 //Num of shifts für Ceasar
#define USER_INPUT_SIZE 50
#define CHAR_COUNT ((int)('z' - 'a') + 1) // Number of chars in Alphabet

static void encode(char *input, char *output, int outputSize, int shiftNum);
static void decode(char *input, char *output, int outputSize, int shiftNum);

static void codingWrapper(struct ceasar_dev *dev, int count, char *bufferTo, char *from);
static int buffer_getwritespace(struct buffer_pipe *dev, struct file *filp);
static int spacefree(struct buffer_pipe *dev);

/*
 * Open and close
 */

int ceasar_open(struct inode *inode, struct file *filp) {
        struct ceasar_dev *dev; /* device information */
        struct buffer_pipe *buff = buffer_p;
        PDEBUG("open called\n");
        //PDEBUG("ReadingSlots: [%d], WritingSlots: [%d]", atomic_read(&reading_slots) - 1, atomic_read(&writing_slots) - 1);
        if ((filp->f_mode & FMODE_READ) && !atomic_dec_and_test(&reading_slots)) {
                PDEBUG("not enough reading slots.. [%d]\n", atomic_read(&reading_slots));
                atomic_inc(&reading_slots);
                return -EBUSY;
        }
        if ((filp->f_mode & FMODE_WRITE) && !atomic_dec_and_test(&writing_slots)) {
                atomic_inc(&writing_slots);
                PDEBUG("not enough writing slots..\n");
                return -EBUSY;
        }

        dev = container_of(inode->i_cdev, struct ceasar_dev, cdev);
        filp->private_data = dev; /* for other methods */
        
        if (!buff->buffer) {
                PDEBUG("Buffer isnt already allocated. Load and Unload the module\n");
                return -ERESTART;
        }
        
        PDEBUG("Succesfully opened device [%d] \n", MINOR(dev->cdev.dev));
        return 0; /* success */
}

int ceasar_release(struct inode *inode, struct file *filp) {
        PDEBUG("Releasing \n");
        if (filp->f_mode & FMODE_READ) {


                atomic_inc(&reading_slots); 
        }
        if (filp->f_mode & FMODE_WRITE)
                atomic_inc(&writing_slots);
        if ((atomic_read(&reading_slots) == NUMBER_OF_READERS) && (atomic_read(&writing_slots) == NUMBER_OF_WRITERS)) {
                PDEBUG("no reader or writer attached anmymore\n");
        }

        return 0;
}

/*
 * Data management: read and write
 */

ssize_t ceasar_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
        struct buffer_pipe *dev = buffer_p;
        if (down_interruptible(&dev->sem)) {
                return -ERESTARTSYS;
        }
        while (dev->rp == dev->wp) {              /* nothing to read */
                up(&dev->sem);                    /* release the lock */
                if (filp->f_flags & O_NONBLOCK) { // Bei non - blocking wird nicht gewartet sondern direkt returned
                        return -EAGAIN;
                }

                PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
                if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp))){ // Returns 0 if the Condition evaluated to true..
                                                                              //Also wenn sich der WritePointer weiterbewegt hat.
                                                                              //Returns somtething else if the process received a signal..
                        return -ERESTARTSYS;    
                }                              /* signal: tell the fs layer to handle it */
                /* otherwise loop, but first reacquire the lock */
                if (down_interruptible(&dev->sem)) //? Könnte sein dass jemand anders ebenfalls gewartet hat und dem Semaphoren hat. Also erst den Semaphoren holen
                        return -ERESTARTSYS;
        }
        /* ok, data is there, return something */
        if (dev->wp > dev->rp) {
                count = min(count, (size_t)(dev->wp - dev->rp));
        } else { /* the write pointer has wrapped, return data up to dev->end */
                count = min(count, (size_t)(dev->end - dev->rp));
        }

        char tmp_buffer[count]; //Decoding/encoding buffer

        codingWrapper(filp->private_data, count, tmp_buffer, dev->rp);

        PDEBUG("Trying to copy to user\n");
        if (copy_to_user(buf, tmp_buffer, count)) { //? Könnte uns auch schlafen legen während wir den Semaphoren haben. Aber in diesem fall gerechtfertig
                up(&dev->sem);
                return -EFAULT;
        }
        PDEBUG("Copy Finished\n");
        dev->rp += count;
        if (dev->rp == dev->end)
                dev->rp = dev->buffer; /* wrapped */
        up(&dev->sem);

        /* finally, awake any writers and return */
        wake_up_interruptible(&dev->outq);
        PDEBUG("Waked up writers\n");
        PDEBUG("\"%s\" did read %li bytes\n", current->comm, (long)count);
        return count;
}

static void codingWrapper(struct ceasar_dev *dev, int count, char *bufferTo, char *from) {
        switch (MINOR(dev->cdev.dev)) {
        case 0:
                encode(from, bufferTo, count, KRYPTO_KEY);
                PDEBUG("Encoded [%d] bytes\n", count);
                break;
        case 1:
                decode(from, bufferTo, count, KRYPTO_KEY);
                PDEBUG("decoded [%d] bytes\n", count);
                break;
        default:
                PDEBUG("Wrong minor Num [%d] while device tried to read\n", MINOR(dev->cdev.dev));
                break;
        }
}

ssize_t ceasar_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
        struct buffer_pipe *dev = buffer_p;
        int result;

        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

        /* Make sure there's space to write */
        result = buffer_getwritespace(dev, filp);
        if (result) {
                return result; /* buffer_getwritespace called up(&dev->sem) */
        }
        /* ok, space is there, accept something */
        count = min(count, (size_t)spacefree(dev));
        if (dev->wp >= dev->rp)
                count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
        else                                                      /* the write pointer has wrapped, fill up to rp-1 */
                count = min(count, (size_t)(dev->rp - dev->wp - 1));

        PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);

        char tmp_buffer[count]; //Decoding/encoding buffer

        if (copy_from_user(tmp_buffer, buf, count)) {
                up(&dev->sem);
                return -EFAULT;
        }

        codingWrapper(filp->private_data, count, dev->wp, tmp_buffer);
        PDEBUG("tried to convert data from Input..[%ld] bytes", count);

        dev->wp += count;
        if (dev->wp == dev->end) {
                dev->wp = dev->buffer; /* wrapped */
                PDEBUG("Write Head wrapped around\n");
        }
        up(&dev->sem);

        /* finally, awake any reader */
        wake_up_interruptible(&dev->inq); /* blocked in read() and select() */

        /* and signal asynchronous readers, explained late in chapter 5 */
        PDEBUG("\"%s\" did write %li bytes\n", current->comm, (long)count);
        return count;
}

/* Wait for space for writing; caller must hold device semaphore.  On
 * error the semaphore will be released before returning. */
static int buffer_getwritespace(struct buffer_pipe *dev, struct file *filp) {
        while (spacefree(dev) == 0) { /* full */
                DEFINE_WAIT(wait);

                up(&dev->sem);
                if (filp->f_flags & O_NONBLOCK)
                        return -EAGAIN;
                PDEBUG("\"%s\" writing: going to sleep\n", current->comm);
                prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
                if (spacefree(dev) == 0)
                        schedule();
                finish_wait(&dev->outq, &wait);
                //if (signal_pending(current))  //TODO fix include
                //       return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
                if (down_interruptible(&dev->sem))
                        return -ERESTARTSYS;
        }
        return 0;
}

/* How much space is free? */
static int spacefree(struct buffer_pipe *dev) {
        if (dev->rp == dev->wp)
                return dev->buffersize - 1;
        return ((dev->rp + dev->buffersize - dev->wp) % dev->buffersize) - 1;
}

struct file_operations ceasar_fops = {
    .owner = THIS_MODULE,
    .read = ceasar_read,
    .write = ceasar_write,
    .open = ceasar_open,
    .release = ceasar_release,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void ceasar_cleanup_module(void) {
        int i;
        dev_t devno = MKDEV(ceasar_major, ceasar_minor); //? create dev num from Major and Minor numbers
        PDEBUG("cleaning up devices\n");
        if (!buffer_p)
                return; /* nothing else to release */

        kfree(buffer_p->buffer);
        buffer_p->buffer = NULL;

        //if(sema_destroy(&buffer_p->sem)){
          //      PDEBUG("Error while destroying sema\n");                
        //}

        kfree(buffer_p);
        buffer_p = NULL; /* pedantic */

        /* Get rid of our char dev entries */
        if (ceasar_devices) {
                for (i = 0; i < ceasar_nr_devs; i++) {
                        //ceasar_trim(ceasar_devices + i); //? Not needed Free the Allocated Mem for each device
                        cdev_del(&ceasar_devices[i].cdev);
                }
                kfree(ceasar_devices);
        }

        /* cleanup_module is never called if registering failed */
        unregister_chrdev_region(devno, ceasar_nr_devs); //? Unregister all Devices. From Start (devno) to ceasar_nr_devs
}

/*
 * Set up the char_dev structure for this device.
 */
static void ceasar_setup_cdev(struct ceasar_dev *dev, int index) {
        int err, devno = MKDEV(ceasar_major, ceasar_minor + index);

        cdev_init(&dev->cdev, &ceasar_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &ceasar_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        /* Fail gracefully if need be */
        if (err)
                printk(KERN_NOTICE "Error %d adding ceasar%d", err, index);
}

int ceasar_init_module(void) {
        int result, i;
        dev_t dev = 0; //? The Device Number..  get major num with MAJOR(dev_t dev)

        /*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */
        //? Dynamic allocation is our way to go
        PDEBUG("buff size is set to [%d]\n", CEASAR_P_BUFFER);

        PDEBUG("trying to register 2 Devices\n");

        //? Else, let the Kernel allocate one for us
        result = alloc_chrdev_region(&dev, ceasar_minor, ceasar_nr_devs, "ceasar");
        ceasar_major = MAJOR(dev);

        //}
        if (result < 0) {
                printk(KERN_WARNING "can't get major %d\n", ceasar_major);
                return result;
        }

        PDEBUG("got Majornum: [%d]\n", ceasar_major);
        /* 
    * allocate the devices -- we can't have them static, as the number
    * can be specified at load time
    */

        ceasar_devices = kmalloc(ceasar_nr_devs * sizeof(struct ceasar_dev), GFP_KERNEL); //? Allocating space for each device
        if (!ceasar_devices) {
                result = -ENOMEM;
                goto fail; /* Make this more graceful */
        }
        PDEBUG("allocated devices\n");
        memset(ceasar_devices, 0, ceasar_nr_devs * sizeof(struct ceasar_dev));

        /* Initialize each device. */
        for (i = 0; i < ceasar_nr_devs; i++) {
                PDEBUG("initing devs (mem not needed) [%d]\n", i);                
                ceasar_setup_cdev(&ceasar_devices[i], i);
        }

        //? Init device shared buffer

        buffer_p = kmalloc(sizeof(struct buffer_pipe), GFP_KERNEL);
        if (buffer_p == NULL) {
                PDEBUG("Couldnt allocate buffer pipe mem\n");
                goto fail;
                return 0;
        }
        memset(buffer_p, 0, sizeof(struct buffer_pipe));

        init_waitqueue_head(&(buffer_p->inq));
        init_waitqueue_head(&(buffer_p->outq));
        init_MUTEX(&buffer_p->sem);

        PDEBUG("allocating buffer buffer\n");
        buffer_p->buffer = kmalloc(CEASAR_P_BUFFER, GFP_KERNEL);

        if (!buffer_p->buffer) {
                //up(&buffer_p->sem); //? freigeben des Semaphoren
                PDEBUG("kmalloc failed @init\n");
                result = -ENOMEM;
                goto fail;
        }

        buffer_p->buffersize = CEASAR_P_BUFFER;
        buffer_p->end = buffer_p->buffer + buffer_p->buffersize;
        buffer_p->rp = buffer_p->wp = buffer_p->buffer; /* rd and wr from the beginning */

        return 0; /* succeed */

fail:
        ceasar_cleanup_module();
        return result;
}

static void encode(char *input, char *output, int outputSize, int shiftNum) {
        int lastIndex = 0;
        if (shiftNum < 0) {
                shiftNum = shiftNum * -1;
        }
        for (int i = 0; (input[i] != '\0') && (i < (outputSize - 1) && (input[i] != '\n')); i++) {
                char chr = input[i];
                if ((chr >= 'a') && (chr <= 'z')) {
                        output[i] = ((input[i] - 'a' + shiftNum) % (CHAR_COUNT)) + 'a';
                } else if ((chr >= 'A') && (chr <= 'Z')) {
                        output[i] = ((input[i] - 'A' + shiftNum) % (CHAR_COUNT)) + 'A';
                } else {
                        output[i] = input[i];
                }
                lastIndex = i;
        }

        output[lastIndex + 1] = '\0';
}

static void decode(char *input, char *output, int outputSize, int shiftNum) {
        encode(input, output, outputSize, CHAR_COUNT - shiftNum);
}

module_init(ceasar_init_module);
module_exit(ceasar_cleanup_module);
