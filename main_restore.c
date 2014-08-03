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
	char *file = argv[1];
	char arg[100];

	int pid;

	if((fd=open("/proc/restore",O_WRONLY|O_APPEND))<0)
      		return -1;
	

	n=write(fd,file,100);
	if(n == 1)
		printf("OK!\n");
	else
		printf("NOT OK!\n");
}
