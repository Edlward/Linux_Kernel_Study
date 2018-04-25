#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <linux/miscdevice.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/delay.h>

#include <asm/hardware.h>

#define DEVICE_NAME	"buttons"
#define BUTTON_MAJOR 232

static struct key_info {
	int irq_no;
	unsigned int gpio_port;
	int key_no;
} key_info_tab[4] = {
	{ IRQ_EINT1, GPIO_F1, 1 },
	{ IRQ_EINT2, GPIO_F2, 2 },
	{ IRQ_EINT3, GPIO_F3, 3 },
	{ IRQ_EINT7, GPIO_F7, 4 },
};

static int ready = 0;
static int key_value = 0;

/* �����ȴ����� */
static DECLARE_WAIT_QUEUE_HEAD(buttons_wait);

//�жϴ�����
static void buttons_irq(int irq, void *dev_id, struct pt_regs *reg)
{
	struct key_info *k;
	int i;
	int found = 0;
	int up;
	int flags;
	
	
	for (i = 0; i < sizeof key_info_tab / sizeof key_info_tab[1]; i++) 
	{
		k = key_info_tab + i;
		if (k->irq_no == irq) 
		{
			found = 1;
			break;
		}
	}
	if (!found) 
	{
		printk("bad irq %d in button\n", irq);
		return;
	}

	/* �����ǿ����жϴ���������Ϊ���Բ��ù��ж� */
	//save_flags(flags);
	//cli();
	
	/* ���ö˿�ģʽ */
	set_gpio_mode_user(k->gpio_port, GPIO_MODE_IN);
	
	/* �ж��ǰ��������ǰ��� */
	up = read_gpio_bit(k->gpio_port);
	
	/* �����ж�ģʽ��ͬʱҲ������жϱ�־λ */
	set_external_irq(k->irq_no, EXT_BOTH_EDGES, GPIO_PULLUP_DIS);
	//restore_flags(flags);
        if (up) 
        	key_value = k->key_no + 0x80;
        else 
        	key_value = k->key_no;

       	ready = 1;
       	
       	/* ���ѵȴ��Ľ��� */
	wake_up_interruptible(&buttons_wait);
}

//ע���ж�
static int request_irqs(void)
{
	struct key_info *k;
	int i;
	/* Ϊ4�����ֱ�ע���ж� */
	for (i = 0; i < sizeof key_info_tab / sizeof key_info_tab[1]; i++) 
	{
		k = key_info_tab + i;
		/* �����жϱ�־������������óɵ����жϣ�����ʹ�������������ʲôӰ��? */
		set_external_irq(k->irq_no, EXT_BOTH_EDGES, GPIO_PULLUP_DIS);
		
		if (request_irq(k->irq_no, &buttons_irq, SA_INTERRUPT, DEVICE_NAME, &buttons_irq)) 
			return -1;
	}
	return 0;
}

static void free_irqs(void)
{
	struct key_info *k;
	int i;
	for (i = 0; i < sizeof key_info_tab / sizeof key_info_tab[1]; i++) 
	{
		k = key_info_tab + i;
		free_irq(k->irq_no, buttons_irq);
	}
}

static int sbc2410_buttons_read(struct file * file, char * buffer, size_t count, loff_t *ppos)
{
	static int key;
	int flags;
	int repeat;
	
	/* �ж��Ƿ������ݿɶ� */
        if (!ready)
        	return -EAGAIN;
        
        if (count != sizeof key_value)
                return -EINVAL;
	
	/*
	save_flags(flags);
	
	if (key != key_value) 
	{
		key = key_value;
		repeat = 0;
	} 
	else 
	{
		repeat = 1;
	}
	
	
	restore_flags(flags);

	if (repeat) {
		return -EAGAIN;
	}
	*/
	key = key_value;
	
        copy_to_user(buffer, &key, sizeof key);
        
        /* ���׼���ñ�־ */
        ready = 0;
        return sizeof key_value;
}

static unsigned int sbc2410_buttons_select(
        struct file *file,
        struct poll_table_struct *wait)
{
	/* �ж��Ƿ��а����жϲ��� */
        if (ready)
                return 1;
        
        poll_wait(file, &buttons_wait, wait);
        return 0;
}


static struct file_operations sbc2410_buttons_fops = {
	owner:	THIS_MODULE,
	poll: sbc2410_buttons_select,
	read: sbc2410_buttons_read,
};

static devfs_handle_t devfs_handle;
static int __init sbc2410_buttons_init(void)
{
	int ret;

	ready = 0;
	
	/* ע���ַ��豸�������� */
	ret = register_chrdev(BUTTON_MAJOR, DEVICE_NAME, &sbc2410_buttons_fops);
	if (ret < 0) {
	  printk(DEVICE_NAME " can't register major number\n");
	  return ret;
	}
	
	/* ע���жϴ������ */
	ret = request_irqs();
	if (ret) {
		unregister_chrdev(BUTTON_MAJOR, DEVICE_NAME);
		printk(DEVICE_NAME " can't request irqs\n");
		return ret;
	}
	
	/* �����豸�ڵ� */
	devfs_handle = devfs_register(NULL, DEVICE_NAME, DEVFS_FL_DEFAULT,
				BUTTON_MAJOR, 0, S_IFCHR | S_IRUSR | S_IWUSR, &sbc2410_buttons_fops, NULL);

	return 0;
}

static void __exit sbc2410_buttons_exit(void)
{
	devfs_unregister(devfs_handle);
	free_irqs();
	unregister_chrdev(BUTTON_MAJOR, DEVICE_NAME);
}

module_init(sbc2410_buttons_init);
module_exit(sbc2410_buttons_exit);
MODULE_LICENSE("GPL");

