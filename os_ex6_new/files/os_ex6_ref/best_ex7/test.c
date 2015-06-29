#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>

#define DEVICE "/dev/simple_char_dev"

int main(int argc, char **argv) {
  int pid = fork();
  assert(pid >= 0);
  
  int fd = open(DEVICE, O_RDONLY);
  int i = 0;
  assert(fd >0);
  
  struct timeval stop, start;
  gettimeofday(&start, NULL);
  
  char buf[101];
  buf[100] = '\0';
  for (i =0; i<100; i++) {
    if ( read(fd, buf+i, 1) != 1)
      break;
  }
  close(fd);
  
  gettimeofday(&stop, NULL);  
  printf("%s (took %lu)\n", buf, stop.tv_usec - start.tv_usec);
  //we can see that the time is different between the two processes (due to the spinlock mechanism)
  
  return 0;
}