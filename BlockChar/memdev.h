#ifndef _MEMDEV_H_
#define _MEMDEV_H_

#include <linux/ioctl.h>

#ifndef MEMDEV_MAJOR
#define MEMDEV_MAJOR 0   
#endif

#ifndef MEMDEV_NR_DEVS
#define MEMDEV_NR_DEVS 3    
#endif

#ifndef MEMDEV_SIZE
#define MEMDEV_SIZE 4096
#endif


typedef struct Mem_Dev {
   char *data;
   struct Mem_Dev *next;   /* next listitem */
   unsigned long size;  
   unsigned long wp, rp;  /* ��д���ݵ�λ�� */
   struct semaphore sem;    /* �����ź��� */
   wait_queue_head_t inq;  /* ��Ҫʹ��ָ�뷽ʽ������Ҫ��kmalloc��Ϊ�����ռ� */
} Mem_Dev;

/* ������� */
#define MEMDEV_IOC_MAGIC  'k'


/* �������� */
#define MEMDEV_IOCPRINT   _IO(MEMDEV_IOC_MAGIC, 1)
#define MEMDEV_IOCGETDATA _IOR(MEMDEV_IOC_MAGIC, 2, int)
#define MEMDEV_IOCSETDATA _IOW(MEMDEV_IOC_MAGIC, 3, int)


#define MEMDEV_IOC_MAXNR 3

#endif /* _MEMDEV_H_ */
