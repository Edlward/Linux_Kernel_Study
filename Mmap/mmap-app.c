#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

int main()
{
	int fd;
	char *start;
	char buf[100]={' '};
	int DATA_LEN = sizeof(buf);
	/*���ļ�*/
	fd = open("testfile",O_CREAT|O_RDWR,S_IRWXU);	
	write(fd,"18188889999",DATA_LEN);	     
	start=mmap(NULL,DATA_LEN,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
	
	/* �������� */
	strcpy(buf,start);
	printf("start addr= %X\n",(unsigned int)start);	
	printf("old buf = %s\n",buf);

	/* д������ */
	strcpy(start,"Buf Is Not Null!");
	//msync(start, DATA_LEN, MS_SYNC);//����kernel������д�����
	printf("new buf = %s\n",buf);
	munmap(start,DATA_LEN); /*���ӳ��*/
	close(fd);  
	
	return 0;	
}
