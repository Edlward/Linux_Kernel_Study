#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

//#include <linux/config.h>
#include <linux/module.h>

#include <linux/kernel.h>   
#include <linux/init.h>     
#include <linux/slab.h>   
#include <linux/fs.h>       
#include <linux/errno.h>    
#include <linux/types.h>    
#include <linux/proc_fs.h>

#include <asm/system.h>  
#include <asm/uaccess.h>   /*ʹ��copy_to_user,copy_from_user����Ҫ�������ļ�*/

#include "memdev.h"

MODULE_LICENSE("GPL");

Mem_Dev *mem_devices; 
int memdev_major = MEMDEV_MAJOR;

/*�豸��*/
int memdev_open(struct inode *inode, struct file *filp)
{
    Mem_Dev *dev;
    
    /*��ȡ���豸��*/
    int num = MINOR(inode->i_rdev);

    dev = (Mem_Dev *)filp->private_data;
    if (!dev) 
    {
        if (num >= MEMDEV_NR_DEVS) 
            return -ENODEV;
        dev = &mem_devices[num];
        filp->private_data = dev; 
    }
    
    /*����ģ�����ü���*/
    //MOD_INC_USE_COUNT; 
	try_module_get(THIS_MODULE);
    return 0;          
}

/*�豸�ر�*/
int memdev_release(struct inode *inode, struct file *filp)
{
    //MOD_DEC_USE_COUNT;
	module_put(THIS_MODULE);
    return 0;
}

/*�豸������*/
ssize_t memdev_read(struct file *filp, char *buf, size_t count,
                loff_t *f_pos)
{
    Mem_Dev *dev = filp->private_data; 
    int pos = *f_pos;
    ssize_t ret = 0;
    
    /*�ж϶�λ���Ƿ���Ч*/
    if (pos >= dev->size)
        goto out;
    if (pos + count > dev->size)
        count = dev->size - pos;
        
    if (!dev->data)
        goto out;
    
    /*�����ݵ��û��ռ�*/
    if (copy_to_user(buf, &(dev->data[pos]), count)) 
    {
        ret = -EFAULT;
	goto out;
    }
    
    *f_pos += count;
    ret = count;
    
 out:
    return ret;
}

/*�ļ�д����*/
ssize_t memdev_write(struct file *filp, const char *buf, size_t count,
                loff_t *f_pos)
{
    Mem_Dev *dev = filp->private_data;
    int pos = *f_pos;
    ssize_t ret = -ENOMEM;

    /*�ж�дλ���Ƿ���Ч*/
    if (pos >= dev->size)
        goto out;
    if (pos + count > dev->size)
        count = dev->size - pos;
    
    /*���û��ռ�д������*/
    if (copy_from_user(&(dev->data[pos]), buf, count)) 
    {
        ret = -EFAULT;
	goto out;
    }

    *f_pos += count;
    ret = count;

  out:
    return ret;
}


/*�ļ���λ*/
loff_t memdev_llseek(struct file *filp, loff_t off, int whence)
{
    Mem_Dev *dev = filp->private_data;
    loff_t newpos;

    switch(whence) {
      case 0: /* SEEK_SET */
        newpos = off;
        break;

      case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;

      case 2: /* SEEK_END */
        newpos = dev->size -1 + off;
        break;

      default: /* can't happen */
        return -EINVAL;
    }
    if (newpos<0) 
    	return -EINVAL;
    	
    filp->f_pos = newpos;
    return newpos;
}


/*
 * The following wrappers are meant to make things work with 2.0 kernels
 */

struct file_operations memdev_fops = {
    .llseek =     memdev_llseek,
    .read =       memdev_read,
    .write =      memdev_write,
    //.ioctl =      NULL,
    .open =       memdev_open,
    .release =    memdev_release,
};


/*ж�غ���*/
void memdev_cleanup_module(void)
{
    int i;
    
    /*ע���ַ��豸*/
    unregister_chrdev(memdev_major, "memdev");

    /*�ͷ��ڴ�*/
    if (mem_devices) 
    {
        for (i=0; i<MEMDEV_NR_DEVS; i++)
	    kfree(mem_devices[i].data);
        kfree(mem_devices);
    }

}

/*���غ���*/
int memdev_init_module(void)
{
    int result, i;
    
    /*����ģ��owner*/
    //SET_MODULE_OWNER(&memdev_fops);
 
    /*ע���ַ��豸*/
    result = register_chrdev(memdev_major, "memdev", &memdev_fops);
    if (result < 0) 
    {
        printk(KERN_WARNING "mem: can't get major %d\n",memdev_major);
        return result;
    }
    if (memdev_major == 0) 
    	memdev_major = result; 

    /*Ϊ�豸�����ṹ�����ڴ�*/
    mem_devices = kmalloc(MEMDEV_NR_DEVS * sizeof(Mem_Dev), GFP_KERNEL);
    if (!mem_devices) 
    {
        result = -ENOMEM;
        goto fail;
    }
    memset(mem_devices, 0, MEMDEV_NR_DEVS * sizeof(Mem_Dev));
    
    /*Ϊ�豸�����ڴ�*/
    for (i=0; i < MEMDEV_NR_DEVS; i++) 
    {
        mem_devices[i].size = MEMDEV_SIZE;
        mem_devices[i].data = kmalloc(MEMDEV_SIZE, GFP_KERNEL);
        memset(mem_devices[i].data, 0, MEMDEV_SIZE);
    }
    
    return 0;

  fail:
    memdev_cleanup_module();
    return result;
}


module_init(memdev_init_module);
module_exit(memdev_cleanup_module);
