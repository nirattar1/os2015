/* Declare what kind of code we want from the header files
   Defining __KERNEL__ and MODULE allows us to access kernel-level 
   code not usually available to userspace programs. */
#undef __KERNEL__
#define __KERNEL__ /* We're part of the kernel */
#undef MODULE
#define MODULE     /* Not a permanent part, though. */

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <asm/uaccess.h>    /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 80
#define BUF_INITIAL "Nir Attar"
#define BUF_INITIAL_LEN 9
#define DEVICE_FILE_NAME "simple_char_dev"



//for debug
#define DEBUG

#ifdef DEBUG
	#define IS_DEBUG 1
#else
	#define IS_DEBUG 0
#endif

#ifdef DEBUG
# define DEBUG_PRINT(x) printk x
#else
# define DEBUG_PRINT(x) do {} while (0)
#endif





struct chardev_info{
    spinlock_t lock;
};



static int dev_open_flag = 0; /* used to prevent concurent access into the same device */
static struct chardev_info device_info;

static char Message[BUF_LEN]; /* The message the device will give when asked */
static int major; /* device major number */

static int last_written = BUF_LEN;

/***************** char device functions *********************/

/* process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    DEBUG_PRINT(("device_open(%p)\n", file));

    /* 
     * We don't want to talk to two processes at the same time 
     */
    spin_lock_irqsave(&device_info.lock, flags);
    if (dev_open_flag){
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }

    dev_open_flag++;
    spin_unlock_irqrestore(&device_info.lock, flags); 

    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file)
{
    unsigned long flags; // for spinlock
    DEBUG_PRINT(("device_release(%p,%p)\n", inode, file));

    /* ready for our next caller */
    spin_lock_irqsave(&device_info.lock, flags);
    dev_open_flag--;
    spin_unlock_irqrestore(&device_info.lock, flags);

    return SUCCESS;
}

/* a process which has already opened 
   the device file attempts to read from it */
static ssize_t device_read(struct file *file, /* see include/linux/fs.h   */
               char __user * buffer, /* buffer to be filled with data */
               size_t length,  /* length of the buffer     */
               loff_t * offset)
{


	char * p; //points to current char.
	char * endRead;
	int num_bytes_read = 0;


	if (buffer==NULL || offset==NULL){ //checking that buffer/offset are not null pointers.
		return -EFAULT;
	}


	DEBUG_PRINT(("initial offset: %llu\n", (long long) *offset));

	//find start point (where previous call had left)
	if (*offset == 0)
	{
		p = Message;
	}
	else
	{
		p = Message + *offset;
	}


	//read only until end of buffer, or amount asked (the lesser of them).
	if ((p+length) >= Message+BUF_LEN )
	{
		endRead = (Message+BUF_LEN);
	}
	else
	{
		endRead = p+length;
	}


	//not reaching end and also not past last write size.
	while (p < endRead && (p<(Message+last_written)) ){
		if (put_user(*p, buffer+num_bytes_read) != 0)	//checking buffer is ok to write.
		{
			return -EFAULT;
		}
		DEBUG_PRINT(("current char was %c)\n", *p));
		p++;
		num_bytes_read++;
		(*offset)++;	//for caller + next calls of this driver.

	}

	//reset offset if reached end.
	if (p==(Message + BUF_LEN))	{
		*offset = 0;
	}

	return num_bytes_read;
}

/* somebody tries to write into our device file */
static ssize_t
device_write(struct file *file,
         const char __user * buffer, size_t length, loff_t * offset)
{

    char TempBuffer[BUF_LEN]; //temporary buff.
    int i=0;
    int num_bytes_wrote;

    DEBUG_PRINT(("device_write(%p,%d)\n", file, (int) length));

	//checking that buffer/offset are not null pointers.
	if (buffer==NULL || offset==NULL){
		return -EFAULT;
	}

    //copy from user buf. to temp.
    //will write Maximum of chars (BUF_LEN - 1).

    for (i = 0; i < length && i < (BUF_LEN-1); i++){

    	if(get_user(TempBuffer[i], buffer++))
		{
			DEBUG_PRINT(("corrupt user buffer.\n"));
			return -EIO;
		}

		DEBUG_PRINT(("writing 1 char: %c\n", TempBuffer[i]));
    }

    num_bytes_wrote = i;
    //if here then tmp was ok. now fill real buffer.
	for (i = 0; i < num_bytes_wrote; i++){
		Message[i] = TempBuffer[i];
		DEBUG_PRINT(("writing 1 char: %c\n", Message[i]));
	}

	//i indicates is last char written.
	//null-terminate buffer.
	Message[i] = '\0';

	*offset = 0; //update offset for caller
	last_written = num_bytes_wrote;

    /* return the number of input characters used */
    return num_bytes_wrote;


}

/************** Module Declarations *****************/

/* This structure will hold the functions to be called
 * when a process does something to the device we created */
struct file_operations Fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,    /* a.k.a. close */
};

/* Called when module is loaded. 
 * Initialize the module - Register the character device */
static int __init simple_init(void)
{
    /* init dev struct*/
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);    

    /* Register a character device. Get newly assigned major num */
    major = register_chrdev(0, DEVICE_RANGE_NAME, &Fops /* our own file operations struct */);

    /* 
     * Negative values signify an error 
     */
    if (major < 0) {
        printk(KERN_ALERT "%s failed with %d\n",
               "Sorry, registering the character device ", major);
        return major;
    }

    printk("Registeration is a success. The major device number is %d.\n", major);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, major);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and rmmod when you're done\n");


    //init message buffer.
    strcpy(Message, BUF_INITIAL) ; //include \0 at end


    return 0;
}

/* Cleanup - unregister the appropriate file from /proc */
static void __exit simple_cleanup(void)
{
    /* 
     * Unregister the device 
     * should always succeed (didnt used to in older kernel versions)
     */
    unregister_chrdev(major, DEVICE_RANGE_NAME);
}

module_init(simple_init);
module_exit(simple_cleanup);
