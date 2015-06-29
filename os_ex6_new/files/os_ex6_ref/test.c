#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

int main(){
	printf("Trying to open /dev/simple_char_dev..\n");
	int fd1 = open("/dev/simple_char_dev", O_RDONLY);
	if(fd1 < 0){
		printf("open failed.\ntest.c failed\n");
		return -1;
	}
	printf("opened succesfully\n\n");

	// now we'll try to open device 2 and make sure we cannot open it because driver is busy

	printf("Trying to open /dev/simple_char_dev1..\n");
	int fd2 = open("/dev/simple_char_dev1", O_RDONLY);
	if(fd2 >= 0){
		printf("open succeded.\ntest.c failed\n");
		if(close(fd1) < 0)
			printf("close() failed\n");
		if(close(fd2) < 0)
			printf("close() failed\n");
		return -1;
	}

	if(errno == 16){		//EBUSY
		printf("open failed.\ntest.c succeded\n\n");
		printf("We got errno : Device or resource busy, as excpected\n");
	}
	else
		printf("test.c failed\n");

	if(close(fd1) < 0)
		printf("close() failed\n");
	return 0;

}

