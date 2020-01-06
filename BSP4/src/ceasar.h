/**
 * definitions for char module
 */

#ifndef _CEASAR_H_
#define _CEASAR_H_

#include <linux/ioctl.h> /* needed for the _IOW stuff used later */

#ifndef init_MUTEX
#define init_MUTEX(mutex) sema_init(mutex, 1)
#endif /*init_MUTEX*/

/**
 * macros for debugging
 */
#undef PDEBUG /*undef it, just to ensure*/
#ifdef CEASAR_DEBUG
#ifdef __KERNEL__
/* this one if debugging on and kernel space*/
#define PDEBUG(fmt, args---) printk(KERN_DEBUG "ceasar: " fmt, ##args)
#else 
/*this one for user space */
#define PDEBUG(fmt, args...) fprintf(stderr, fmt, ##args)
#endif
#else
#define PDEBUG(fmt, args...) /* placeholder*/
#endif

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /*placeholder*/

#ifndef CEASAR_MAJOR
#define CEASAR_MAJOR 0 /* dynamic major by default */
#endif

#ifndef CEASAR_NR_DEVS
#define CEASAR_NR_DEVS 2 /* ceasar0 through ceasar 3 */ //number of needed devices
#endif

#ifndef CEASAR_P_NR_DEVS
#define CEASAR_P_NR_DEVS 4 /* ceasarpipe 0 throug ceasarpipe3 */
#endif

/*
 * The bare device is a variable-length region of memory.
 * Use a linked list of indirect blocks.
 *
 * "ceasar_dev->data" points to an array of pointers, each
 * pointer refers to a memory area of CEASAR_QUANTUM bytes.
 *
 * The array (quantum-set) is CEASAR_QSET long.
 */
#ifndef CEASAR_QUANTUM 
#define CEASAR_QUANTUM 4000
#endif

#ifndef CEASAR_QSET
#define CEASAR_QSET 1000
#endif

/**
 * the pipe device is simple circular buffer, here its default size
 */
#ifndef CEASAR_P_BUFFER
#define CEASAR_P_BUFFER 4000
#endif

/**
 * representation of ceasar quantum sets
 */
struct ceasar_qset {
    void **data;
    struct ceasar_qset *next;
};

struct ceasar_dev {
    struct ceasar_qset *data;   /*pointer to first quantum set*/
    int quantum;                //current quantum set
    int qset;                   //current array size
    unsigned long size;         //amount of data stored here
    unsigned int access_key;     //used by ceasaruid and ceasarpriv
    struct semaphore sem;       /* mutual exclusion semaphore*/
    struct cdev cdev;           //char dvice structure
};

/**
 * split minors in two parts
 */
#define TYPE(minor) ((minor) >> 4) & 0xf) //high nibble
#define NUM(minor) ((minor) & 0xf)        //low nibble

/**
 * different configurable parameters
 */
extern int ceasar_major; //main.c
extern int ceasar_nr_devs;
extern int ceasar_quantum;
extern int ceasar_qset;

extern int ceasar_p_buffer; //pipe.c

/**
 * prototypes for shared functions
 */
int ceasar_p_init(dev_t dev);
void ceasar_p_cleanup(void);
int ceasar_access_init(dev_t dev);
void ceasar_access_cleanup(void);

int ceasar_trim(struct ceasar_dev *dev);

ssize_t ceasar_read(struct file *filp, char __user *buf, size_t count,
                    loff_t *f_pos);
ssize_t ceasar_write(struct file *filp, const char __user *buf, size_t count, 
                     loff_t *f_pos);
loff_t ceasar_llseek(struct file *filp, loff_t off, int whence);
int ceasar_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg);

/**
 * ioctl definitions
 */

//use 'k' as magic number
#define CEASAR_IOC_MAGIC 'k'
//please use different 8-bit number in your code

#define CEASAR_IOCRESET _IO(CEASAR_IOC_MAGIC, 0)

/**
 * S means "Set" through a ptr,
 * T means "Tell" directly with the argument value,
 * G means "Get": reply by setting through a pointer
 * Q means "Query": response is on the return value
 * X means "eXchange": switch G and S atomically
 * H means "sHift"   : switch T and Q atomically
 */
#define CEASAR_IOCSQUANTUM _IOW(CEASAR_IOC_MAGIC, 1, int)
#define CEASAR_IOCSQSET    _IOW(CEASAR_IOC_MAGIC, 2, int)
#define CEASAR_IOCTQUANTUM _IO(CEASAR_IOC_MAGIC, 3)
#define CEASAR_IOCTQSET    _IO(CEASAR_IOC_MAGIC, 4)
#define CEASAR_IOCGQUANTUM _IOR(CEASAR_IOC_MAGIC, 5, int)
#define CEASAR_IOCGQSET    _IOR(CEASAR_IOC_MAGIC, 6, int)
#define CEASAR_IOCQQUANTUM _IO(CEASAR_IOC_MAGIC, 7)
#define CEASAR_IOCQQSET    _IO(CEASAR_IOC_MAGIC, 8)
#define CEASAR_IOCXQUANTUM _IOWR(CEASAR_IOC_MAGIC, 9, int)
#define CEASAR_IOCXQSET    _IOWR(CEASAR_IOC_MAGIC, 10, int)
#define CEASAR_IOCHQUANTUM _IO(CEASAR_IOC_MAGIC, 11)
#define CEASAR_IOCHQSET    _IO(CEASAR_IOC_MAGIC, 12)

/**
 * other entities only have "tell" and "query", because they're
 * not printed in the book, and there is no need to have all six
 * (previous stuff was only there to show different ways to do it)
 */
#define CEASAR_P_IOCTSIZE _IO(CEASAR_IOC_MAGIC, 13)
#define CEASAR_P_IOCQSIZE _IO(CEASAR_IOC_MAGIC, 14)
//more to come

#define CEASAR_IOC_MAXNUM 14

#endif /* _CEASAR_H_ */