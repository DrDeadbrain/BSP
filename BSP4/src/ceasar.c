/**
 * most includes from example code
 */

//#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <linux/cdev.h>
#include <linux/errno.h>            //error codes
#include <linux/fcntl.h>            //O_ACCMODE
#include <linux/fs.h>               //everything...
#include <linux/kernel.h>           //printk()
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
//#include <linux/shed/signal.h>    //TODO fix include
#include <linux/slab.h>             //kmalloc()
#include <linux/types.h>            //size_t
#include <linux/uaccess.h>          // copy_*_user
                                    //
//#include <asm/system.h>           //cli(), *_flags
//#include <asm/uaccess.h>          //copy_*_user

#include "ceasar.h"  /*local def.*/

#define NUMBER_OF_READER 1 //1 in this conf
#define NUMBER_OF_WRITER 1

/**
 * parameters that can be set at load time
 */
struct buffer_pipe {
    wait_queue_head_t inq, outq;            /*read and write queues*/
    char *buffer, *end;                     /* begin of buf, end of buf */
    int bufferSize;                         /* used in pointer arithmetic */
    char *rp, *wp;                          /* where to read, where to write */
    int nreader, nwriter;                   /* num of openings for r/w */
    struct fasync_struct *async_queue;      /* async reader */
    struct semaphore sem;                   /* mutual exclusion semaphore to ensure that only one can access*/
    struct cdev cdev;                       /* char device struct */
};

static struct buffer_pipe *buffer_p;

static atomic_t reading_slots = ATOMIC_INIT(NUMBER_OF_READER);
static atomic_t writing_slots = ATOMIC_INIT(NUMBER_OF_WRITER);

int ceasar_major = CEASAR_MAJOR;
int ceasar_minor = 0;               //start minor num for devices
int ceasar_nr_devs = CEASAR_NR_DEVS; //number of bare ceasar devices

module_param(ceasar_major, int, S_IRUGO);
module_param(ceasar_minor, int, S_IRUGO);
module_param(ceasar_nr_devs, int, S_IRUGO);

MODULE_AUTHOR("Christian Caus, Stefan Subotin");
MODULE_LICENSE("Dual BSD/GPL");

struct ceasar_dev *ceasar_devices; //allocated in ceasar_init_module

#define CRYPTO_KEY 1 //num of shifts for ceasar
#define USER_INPUT_SIZE 50 
#define CHAR_COUNT ((int)('z' - 'a') + 1)

static void encode(char *input, char *output, int outputSize, int shiftNum);
static void decode(char *input, char *output, int outputSize, int shiftNum);

static void codingWrapper(struct ceasar_dev *dev, int count, char *bufferTo, char *from);
static int buffer_getwritespace(struct buffer_pipe *dev, struct file *filp);
static int spacefree(struct buffer_pipe *dev);

//open and close
int ceasar_open(struct inode *inode, struct file *filp) {
    struct ceasar_dev *dev; //device info
    struct buffer_pipe *buff = buffer_p;
    PDEBUG("open called\n");
    //PDEBUG("ReadingSlots: [%d], WritingSlots: [%d]", atomic_read(&reading_slots) - 1, atomic_read(&writing_slots) -1 );
    if ((filp->f_mode & FMODE_READ) && !atomic_dec_and_test(&reading_slots)) {
        PDEBUG("not enough reading slots.. [%d]\n", atomic_read(&reading_slots));
        atomic_inc(&reading_slots);
        return -EBUSY;
    }
    if ((filp->f_mode & FMODE_WRITE) && atomic_dec_and_test(&writing_slots)) {
        atomic_inc(&writing_slots);
        PDEBUG("not enough writing slots...");
        return -EBUSY;
    }
    dev = container_of(inode->i_cdev, struct ceasar_dev, cdev);
    filp->private_data = dev; //for other methods

    if (!buff->buffer) {
        PDEBUG("Buffer isn't already allocated. Load and unload module\n");
        return -ERESTART;
    }
    PDEBUG("Successfully opened device [%d]\n", MINOR(dev->cdev.dev));
    return 0; //success
}

int ceasar_release(struct inode *inode, struct file *filp) {
    PDEBUG("Releasing\n");
    if (filp->f_mode & FMODE_READ) {
        atomic_inc(&reading_slots);
    }
    if (filp->f_mode & FMODE_WRITE) {
        atomic_inc(&reading_slots);
    }
    if ((atomic_read(&reading_slots) == NUMBER_OF_READER) && (atomic_read(&writing_slots) == NUMBER_OF_WRITER)) {
        PDEBUG("no reader or writer attached anymore\n");
    }
    return 0;
}

/**
 * data management: read and write
 */
ssize_t ceasar_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
    struct buffer_pipe *dev = buffer_p;
    if (down_interruptible(&dev->sem)) {
        return -ERESTARTSYS;
    }
    while (dev->rp == dev->wp) {  //nothing to read
        up(&dev->sem);            //release lock
        if (filp->f_flags & O_NONBLOCK) {   //non blocking ->instant return
            return -EAGAIN;
        }
        PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
        if (wait_event_interruptible(dev->inq, (dev->rp != dev ->wp))) { //0 if cond true
            return -ERESTARTSYS;                                         //returns something other if signal reveived
        } //signal tell fs layer to handle it
        /* otherwise loop but first reaquire the lock*/
        if (down_interruptible(&dev->sem)) {    //get sem first if someone else waited before
            return -ERESTARTSYS;
        }
    }
    /* ok, data is here, return */
    if (dev->wp > dev->rp) {
        count = min(count, (size_t)(dev->wp - dev->rp));
    } else {    //writer pointer has wrapped, return data up to dev->end
        count = min(count, (size_t)(dev->end - dev->rp));
    }

    char tmp_buffer[count]; //de/encoding buffer

    codingWrapper(filp-> private_data, count, tmp_buffer, dev->rp);

    PDEBUG("Trying to copy user\n");
    if (copy_to_user(buf, tmp_buffer, count)) { //
        up(&dev->sem);
        return -EFAULT;
    }
    PDEBUG("Copy finished\n");
    dev->rp += count;
    if (dev->rp == dev->end) {
        dev->rp = dev->buffer; //wrapped
    }
    up(&dev->sem);

    /* finally, awake any writers */
    wake_up_interruptible(&dev->outq);
    PDEBUG("Waked up writers\n");
    PDEBUG("\"%s\" did read %li bytes\n", current->comm, (long)count);
    return count;
}

static void codingWrapper (struct ceasar_dev *dev, int count, char *bufferTo, char *from) {
    switch (MINOR(dev->cdev.dev)) {
        case 0:
            encode(from, bufferTo, count, CRYPTO_KEY);
            PDEBUG("Encoded [%d] bytes\n", count);
            break;
        case 1:
            decode(from, bufferTo, count, CRYPTO_KEY);
            PDEBUG("Decoded [%d] bytes\n", count);
            break;
        default:
            PDEBUG("Wrong minor number Num [%d] while deviced tried to read\n", MINOR(dev->cdev.dev));
            break;
    }
}

ssize_t ceasar_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
    struct buffer_pipe *dev = buffer_p;
    int result;

    if (down_interruptible(&dev->sem)) {
        return -ERESTARTSYS;
    }
    /* make sure there is space to write */
    result = buffer_getwritespace(dev, filp);
    if (result) {
        return result; //buffer_getwritespace called up(&dev->sem)
    }

    /* ok space is there, accept something */
    count = min(count, (size_t)spacefree(dev));
    if (dev->wp >= dev->rp) {
        count = min(count, (size_t)(dev->end - dev->wp)); //to end of buf
    } else {
        count = min(count, (size_t)(dev->rp - dev->wp - 1));
    }
    PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);

    char tmp_buffer[count]; //decoding/encoding buffer

    if (copy_from_user(tmp_buffer, buf, count)) {
        up(&dev->sem);
        return -EFAULT;
    }

    codingWrapper(filp->private_data, count, dev->wp, tmp_buffer);
    PDEBUG("tried to convert data from Input..[%d] bytes", count);

    dev->wp += count;
    if (dev->wp == dev->end) {
        dev->wp = dev->buffer; //wrapped
        PDEBUG("Write Head wrapped around\n");
    }
    up(&dev->sem);

    /* finally awake any reader */
    wake_up_interruptible(&dev->inq); //blocked in read() and select()

    /* and signal asynchronous readers, explained late in chap 5 */
    PDEBUG("\"%s\" did write %li bytes\n", current->comm, (long)count);
    return count;
}

/**
 * wait for space for writing; caller must hold device semaphore, on 
 * error the semaphore will be released before returning
 */
static int buffer_getwritespace(struct buffer_pipe *dev, struct file *filp) {
    while (spacefree(dev) == 0) { //full
        DEFINE_WAIT(wait);

        up(&dev->sem);
        if (filp->f_flags & O_NONBLOCK) {
            return -EAGAIN;
        }
        PDEBUG("\"%s\"writing: going to sleep\n", current->comm);
        prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
        if (spacefree(dev) == 0) {
            schedule;
        }
        finish_wait(&dev->outq, &wait);
        //if (signal_pending(current)) //TODO fix include
        // return -ERESTARTSYS; /*signal: tell the fs layer to handle it*/
        if (down_interruptible(&dev->sem)) {
            return -ERESTARTSYS;
        }
    }
    return 0;
}

/**
 * method that checks how much space is free
 */
static int spacefree(struct buffer_pipe *dev) {
    if (dev->rp == dev->wp) {
        return dev->bufferSize - 1;
    }
    return ((dev->rp + dev->bufferSize - dev->wp) % dev->bufferSize) - 1;
}

struct file_operations ceasar_fops = {
    .owner   = THIS_MODULE,
    .read    = ceasar_read,
    .write   = ceasar_write,
    .open    = ceasar_open,
    .release = ceasar_release,
};

/**
 * now the module part of the program
 */

/**
 * The cleanup function is used to handle initialization failures as well
 * therefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void ceasar_cleanup_module(void) {
    int i;
    dev_t devno = MKDEV(ceasar_major, ceasar_minor); //create dev num from major and minor num
    PDEBUG("cleaning up devices\n");
    if (!buffer_p) {
        return; //nothing else to release
    }
    kfree(buffer_p->buffer);
    buffer_p->buffer = NULL;

    //if (sema_destroy(&buffer_p->sem)) {
        //PDEBUG("Error while destroying sema")
    //}
    kfree(buffer_p);
    buffer_p->buffer = NULL; //pedantic

    /* get rid of char dev entires */
    if (ceasar_devices) {
        for ( i = 0; i < ceasar_nr_devs; i++) {
            //ceasar_trim(ceasar_devices + i) //?not needed ; free allocated mem for each device
            cdev_del(&ceasar_devices[i].cdev);
        }
        kfree(ceasar_devices);
    }
    /* cleanup module is never called if registering failed */
    unregister_chrdev_region(devno, ceasar_nr_devs); //?unregister all devices
}

/**
 * set up chear_dev structure for this device
 */
static void ceasar_setup_cdev(struct ceasar_dev *dev, int index) {
    int err, devno = MKDEV(ceasar_major, ceasar_minor + index);

    cdev_init(&dev->cdev, &ceasar_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &ceasar_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    /* fail gracefully if needed */
    if (err) {
        printk(KERN_NOTICE "Error %d adding ceasar%d", err, index);
    }
}

/**
 * get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time
 */
int ceasar_init_module(void) {
    int result, i;
    dev_t dev = 0; //device number...get major num with MAJOR(dev_t dev)

    //dynamic allocation is way to go
    PDEBUG("buff size is set to [%d]\n", CEASAR_P_BUFFER);
    PDEBUG("trying to register 2 devices\n");

    //else let kernel allocate for us
    result = alloc_chrdev_region(&dev, ceasar_minor, ceasar_nr_devs, "ceasar");
    ceasar_major = MAJOR(dev);

    if (result < 0 ) {
        printk(KERN_WARNING "can't get major %d\n", ceasar_major);
        return result;
    }
    PDEBUG("got Major num: [%d]\n", ceasar_major);

    /**
     * allocate the devices -- we can't have them static, as the number 
     * can be specified at load time
     */
    ceasar_devices = kmalloc(ceasar_nr_devs * sizeof(struct ceasar_dev), GFP_KERNEL); //allocationg space for each device
    if (!ceasar_devices) {
        result = -ENOMEM;
        goto fail; //make this more graceful
    }
    PDEBUG("allocated devices\n");
    memset(ceasar_devices, 0, ceasar_nr_devs * sizeof(struct ceasar_dev));

    /* init each device */
    for (i = 0; i < ceasar_nr_devs; i++) {
        PDEBUG("init devs (mem not needed) [%d]\n", i);
        ceasar_setup_cdev(&ceasar_devices[i], i);
    }

    /* init device shared buffer */
    buffer_p = kmalloc(sizeof(struct buffer_pipe), GFP_KERNEL);
    if (buffer_p == NULL) {
        PDEBUG("Couldn't allocate buffer pipe mem\n");
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
        //up(&buffer_p->sem)  //?freigeben des semaphores?
        PDEBUG("kmalloc failed @init\n");
        result = -ENOMEM;
        goto fail;
    }
    buffer_p->bufferSize = CEASAR_P_BUFFER;
    buffer_p->end = buffer_p->buffer + buffer_p->bufferSize;
    buffer_p->rp = buffer_p->wp = buffer_p->buffer; //rd and wr from beginning

    return 0; //success

fail:
    ceasar_cleanup_module();
    return result;
}

static void encode(char *input, char *output, int outputSize, int shiftNum) {
    if (shiftNum < 0) {
        shiftNum = shiftNum * -1;
    }
    int lastIdx = 0;

    for (int i = 0; (input[i] != '\0') && (i < (outputSize - 1) && (input[i] != '\n')); i++) {
        char nxtChar = input[i];
        if ((nxtChar >= 'a') && (nxtChar <= 'z')) {
            output[i] = ((input[i] - 'a' + shiftNum) % (CHAR_COUNT)) + 'a';
        } else if ((nxtChar >= 'A') && (nxtChar <= 'Z')) {
            output[i] = ((input[i] - 'A' + shiftNum) % (CHAR_COUNT)) + 'A';
        } else {
            output[i] = input[i];
        }
        lastIdx = i;
    }
    output[lastIdx + 1] = '\0';
}

static void decode(char *input, char *output, int outputSize, int shiftNum) {
    encode(input, output, outputSize, CHAR_COUNT - shiftNum);
}

module_init(ceasar_init_module);
module_exit(ceasar_cleanup_module);



