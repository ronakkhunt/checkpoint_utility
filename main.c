#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
int main(int argc, char **argv)
{
	int n,fd;
	int pid = atoi(argv[1]);

	if((fd=open("/proc/checkpoint",O_WRONLY|O_APPEND))<0)
      		return -1;
	
	n=write(fd,&pid,sizeof(pid));
	if(n == 1)
		printf("OK!\n");
	else
		printf("NOT OK!\n");
}
