/*
 * pipe.c -- fifo driver for buffer
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>

#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/errno.h> /* error codes */
#include <linux/fcntl.h>
#include <linux/fs.h>     /* everything... */
#include <linux/kernel.h> /* printk(), min() */
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>  /* kmalloc() */
#include <linux/types.h> /* size_t */

#include "buffer.h" /* local definitions */

//TODO auslagern:

#define NUMBER_OF_READERS 1
#define NUMBER_OF_WRITERS 1

#define USER_INPUT_SIZE 50
#define CHAR_COUNT ((int)('z' - 'a') + 1) // Number of chars in Alphabet
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

/* parameters */
static int buffer_p_nr_devs = BUFFER_P_NR_DEVS; /* number of pipe devices */ //TODO define finden
int buffer_p_buffer = BUFFER_P_BUFFER; /* buffer size */                     //TODO define finden
dev_t buffer_p_devno;                                                        /* Our first device number */

static atomic_t reading_slots = ATOMIC_INIT(NUMBER_OF_READERS);
static atomic_t writing_slots = ATOMIC_INIT(NUMBER_OF_WRITERS);

module_param(buffer_p_nr_devs, int, 0); /* FIXME check perms */
module_param(buffer_p_buffer, int, 0);

static struct buffer_pipe *buffer_p_devices;

static void encode(char *input, char *output, int outputSize, int shiftNum);
static void decode(char *input, char *output, int outputSize, int shiftNum);

static int buffer_p_fasync(int fd, struct file *filp, int mode);
static int spacefree(struct buffer_pipe *dev);
/*
 * Open and close
 */

static int buffer_p_open(struct inode *inode, struct file *filp) {
        struct buffer_pipe *dev;

        if ((filp->f_mode & FMODE_READ) && !atomic_dec_and_test(&reading_slots)) {
                atomic_inc(&reading_slots);
                PDEBUG(("not enough reading slots..\n", current->comm));
                return -EBUSY;
        }
        if ((filp->f_mode & FMODE_WRITE) && atomic_dec_and_test(&writing_slots)) {
                atomic_inc(&reading_slots);
                PDEBUG(("not enough writing slots..\n", current->comm));
                return -EBUSY;
        }
        dev = container_of(inode->i_cdev, struct buffer_pipe, cdev);
        filp->private_data = dev;

        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;
        if (!dev->buffer) {
                /* allocate the buffer */
                dev->buffer = kmalloc(buffer_p_buffer, GFP_KERNEL);
                if (!dev->buffer) {
                        up(&dev->sem); //? freigeben des Semaphoren
                        return -ENOMEM;
                }
        }
        dev->buffersize = buffer_p_buffer;
        dev->end = dev->buffer + dev->buffersize;
        dev->rp = dev->wp = dev->buffer; /* rd and wr from the beginning */

        up(&dev->sem);

        return nonseekable_open(inode, filp);
}

static int buffer_p_release(struct inode *inode, struct file *filp) {
        struct buffer_pipe *dev = filp->private_data; //? Warum geht das hier ohne cast.. private data ist einfach ein void pointer in dem man etwas speichern kann

        /* remove this filp from the asynchronously notified filp's */
        buffer_p_fasync(-1, filp, 0);
        down(&dev->sem);
        if (filp->f_mode & FMODE_READ)
                atomic_inc(&reading_slots);
        if (filp->f_mode & FMODE_WRITE)
                atomic_inc(&writing_slots);
        if (atomic_read((&reading_slots) == NUMBER_OF_READERS) && (atomic_read(&writing_slots) == NUMBER_OF_WRITERS)) {
                kfree(dev->buffer);
                dev->buffer = NULL; /* the other fields are not checked on open */
        }
        up(&dev->sem);
        return 0;
}

/*
 * Data management: read and write
 */

static ssize_t buffer_p_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
        struct buffer_pipe *dev = filp->private_data;

        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

        while (dev->rp == dev->wp) {            /* nothing to read */
                up(&dev->sem);                  /* release the lock */
                if (filp->f_flags & O_NONBLOCK) // Bei non - blocking wird nicht gewartet sondern direkt returned
                        return -EAGAIN;
                PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
                if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp))) // Returns 0 if the Condition evaluated to true..
                                                                              //Also wenn sich der ReadPointer weiterbewegt hat.
                                                                              //Returns somtething else if the process received a signal..
                        return -ERESTARTSYS;                                  /* signal: tell the fs layer to handle it */
                /* otherwise loop, but first reacquire the lock */
                if (down_interruptible(&dev->sem)) //? Könnte sein dass jemand anders ebenfalls gewartet hat und dem Semaphoren hat. Also erst den Semaphoren holen
                        return -ERESTARTSYS;
        }
        /* ok, data is there, return something */
        if (dev->wp > dev->rp)
                count = min(count, (size_t)(dev->wp - dev->rp));
        else /* the write pointer has wrapped, return data up to dev->end */
                count = min(count, (size_t)(dev->end - dev->rp));
        if (copy_to_user(buf, dev->rp, count)) { //? Könnte uns auch schlafen legen während wir den Semaphoren haben. Aber in diesem fall gerechtfertig
                up(&dev->sem);
                return -EFAULT;
        }
        dev->rp += count;
        if (dev->rp == dev->end)
                dev->rp = dev->buffer; /* wrapped */
        up(&dev->sem);

        /* finally, awake any writers and return */
        wake_up_interruptible(&dev->outq);
        PDEBUG("\"%s\" did read %li bytes\n", current->comm, (long)count);
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
                if (signal_pending(current))
                        return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
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

static ssize_t buffer_p_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
        struct buffer_pipe *dev = filp->private_data;
        int result;

        if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

        /* Make sure there's space to write */
        result = buffer_getwritespace(dev, filp);
        if (result)
                return result; /* buffer_getwritespace called up(&dev->sem) */

        /* ok, space is there, accept something */
        count = min(count, (size_t)spacefree(dev));
        if (dev->wp >= dev->rp)
                count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
        else                                                      /* the write pointer has wrapped, fill up to rp-1 */
                count = min(count, (size_t)(dev->rp - dev->wp - 1));
        PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
        if (copy_from_user(dev->wp, buf, count)) {
                up(&dev->sem);
                return -EFAULT;
        }
        dev->wp += count;
        if (dev->wp == dev->end)
                dev->wp = dev->buffer; /* wrapped */
        up(&dev->sem);

        /* finally, awake any reader */
        wake_up_interruptible(&dev->inq); /* blocked in read() and select() */

        /* and signal asynchronous readers, explained late in chapter 5 */
        if (dev->async_queue)
                kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
        PDEBUG("\"%s\" did write %li bytes\n", current->comm, (long)count);
        return count;
}

static unsigned int buffer_p_poll(struct file *filp, poll_table *wait) {
        struct buffer_pipe *dev = filp->private_data;
        unsigned int mask = 0;

        /*
	 * The buffer is circular; it is considered full
	 * if "wp" is right behind "rp" and empty if the
	 * two are equal.
	 */
        down(&dev->sem);
        poll_wait(filp, &dev->inq, wait);
        poll_wait(filp, &dev->outq, wait);
        if (dev->rp != dev->wp)
                mask |= POLLIN | POLLRDNORM; /* readable */
        if (spacefree(dev))
                mask |= POLLOUT | POLLWRNORM; /* writable */
        up(&dev->sem);
        return mask;
}

static int buffer_p_fasync(int fd, struct file *filp, int mode) {
        struct buffer_pipe *dev = filp->private_data;

        return fasync_helper(fd, filp, mode, &dev->async_queue);
}

/* FIXME this should use seq_file */

/*
 * The file operations for the pipe device
 * (some are overlayed with bare buffer)
 */
struct file_operations buffer_pipe_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = buffer_p_read,
    .write = buffer_p_write,
    .poll = buffer_p_poll,
    .open = buffer_p_open,
    .release = buffer_p_release,
    .fasync = buffer_p_fasync,
};

/*
 * Set up a cdev entry.
 */
static void buffer_p_setup_cdev(struct buffer_pipe *dev, int index) {
        int err, devno = buffer_p_devno + index;

        cdev_init(&dev->cdev, &buffer_pipe_fops);
        dev->cdev.owner = THIS_MODULE;
        err = cdev_add(&dev->cdev, devno, 1);
        /* Fail gracefully if need be */
        if (err)
                printk(KERN_NOTICE "Error %d adding bufferpipe%d", err, index);
}

/*
 * Initialize the pipe devs; return how many we did.
 */
int buffer_p_init(dev_t firstdev) {
        int i, result;

        result = register_chrdev_region(firstdev, buffer_p_nr_devs, "bufferp");
        if (result < 0) {
                printk(KERN_NOTICE "Unable to get bufferp region, error %d\n", result);
                return 0;
        }
        buffer_p_devno = firstdev;
        buffer_p_devices = kmalloc(buffer_p_nr_devs * sizeof(struct buffer_pipe), GFP_KERNEL);
        if (buffer_p_devices == NULL) {
                unregister_chrdev_region(firstdev, buffer_p_nr_devs);
                return 0;
        }
        memset(buffer_p_devices, 0, buffer_p_nr_devs * sizeof(struct buffer_pipe));
        for (i = 0; i < buffer_p_nr_devs; i++) {
                init_waitqueue_head(&(buffer_p_devices[i].inq));
                init_waitqueue_head(&(buffer_p_devices[i].outq));
                init_MUTEX(&buffer_p_devices[i].sem);
                buffer_p_setup_cdev(buffer_p_devices + i, i);
        }
        return buffer_p_nr_devs;
}

/*
 * This is called by cleanup_module or on failure.
 * It is required to never fail, even if nothing was initialized first
 */
void buffer_p_cleanup(void) {
        int i;

        if (!buffer_p_devices)
                return; /* nothing else to release */

        for (i = 0; i < buffer_p_nr_devs; i++) {
                cdev_del(&buffer_p_devices[i].cdev);
                kfree(buffer_p_devices[i].buffer);
        }
        kfree(buffer_p_devices);
        unregister_chrdev_region(buffer_p_devno, buffer_p_nr_devs);
        buffer_p_devices = NULL; /* pedantic */
}
