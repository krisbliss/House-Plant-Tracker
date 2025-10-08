#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "file.h"
#include "common.h"

int create_db_file(char *filename){
  
  // first check if file already exsits
  int fd = open(filename , O_RDONLY);
  if(fd != -1){
    close(fd);
    printf("%s already exsits.\n\t If this is the correct data base file, then please re-run without the \"-n\" flag\n",filename);
    return STATUS_ERROR;
  }

  fd = open(filename, O_RDWR | O_CREAT, 0664);
  if(fd == -1){
    perror("file open");
    return STATUS_ERROR;
  }
  return fd;
}

int open_db_file(char *filename){
  int fd = open(filename, O_RDWR);
  if(fd == -1){
    perror("file open");
    return STATUS_ERROR;
  }
  return fd;
}
