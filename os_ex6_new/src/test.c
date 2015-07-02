#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

//this test will check that opening 2 device files that point to our driver-
//-- will be prevented.
//the lock mechanism is supposed to prevent concurrent accesses to device.
//the case here is also an option - if someone creates 2 device files for this driver
//(via "mknod" command), then user can open device on 1st file,
//and then through 2nd file without closing it.
//with locking mechanism this is avoided.

int main()
{

	printf("opening file /dev/simple_char_dev\n");
	int fd1 = open("/dev/simple_char_dev", O_RDONLY);

	if(fd1 < 0)
	{
		printf("cannot open 1st device.\n");
		return -1;
	}

	printf("opened 1st device .\n");



	printf("opening 2nd device file (same driver): /dev/simple_char_dev2\n");
	//expecting failure
	int fd2 = open("/dev/simple_char_dev2", O_RDONLY);
	if(fd2 >= 0)
	{
		printf("two files open together. problem!\n");
		close(fd1);
		close(fd2);
		return -1;
	}


	//validate reason for 'open' failure - expecting "Device or resource busy"
	if(errno == 16)
	{
		printf("opening 2nd device file failed due to busy. (expected)\n");
	}
	else
	{
		printf("opening 2nd device file failed -possible error.\n");
	}

	close(fd1);
	return 0;

}

