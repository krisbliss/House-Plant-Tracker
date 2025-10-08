#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "parse.h"
#include "common.h"


int validate_db_header(int fd, struct db_header_t **headerOut){

  // first confirm fd is valid
  if (fd < 0){
    printf("Got a bad file descripter from user\n");
    return STATUS_ERROR;
  }
  
  
  // create tmp header to read file data into
  struct db_header_t *tmp_header = calloc(1, sizeof(struct db_header_t));
  if(tmp_header == NULL){
    perror("validate_db_header : calloc\n");
    return STATUS_ERROR;
  }

  // read data from disk into tmp_header
  if(read(fd, tmp_header, sizeof(struct db_header_t)) != sizeof(struct db_header_t)){
    perror("read");
    free(tmp_header);
    return STATUS_ERROR;
  }

  // unpacks data from disk to host endian
  tmp_header->version = ntohs(tmp_header->version);
  tmp_header->count = ntohs(tmp_header->count);
  tmp_header->magic = ntohl(tmp_header->magic);
  tmp_header->filesize = ntohl(tmp_header->filesize);


  // stat struct used to confirm header size is valid
  struct stat db_stat = {0};
  fstat(fd, &db_stat);

  // compare fstat calculated filesize to filesize stored in header
  if(tmp_header->filesize != db_stat.st_size){
    printf("Corrupted database\n");
    free(tmp_header);
    return STATUS_ERROR;
  }
  
  // check to see if this is a valid 
  if(tmp_header->magic != HEADER_MAGIC){
    printf("File is NOT a database file\n");
    free(tmp_header);
    return STATUS_ERROR;
  }

  // check if version is valid
  if(tmp_header->version != VERSION){
    printf("File version is outdated");
    free(tmp_header);
    return STATUS_ERROR;
  }

  *headerOut = tmp_header;
  printf("Read Success : File is valid\n");
  return STATUS_SUCCESS; 
}


int create_db_header(int fd, struct db_header_t **headerOut){
  // init header structer
  struct db_header_t *tmp_header = calloc(1,sizeof(struct db_header_t));
  if(tmp_header == NULL){
    perror("calloc");
    return STATUS_ERROR;
  }
  
  tmp_header->version = VERSION;
  tmp_header->count = 0;
  tmp_header->magic = HEADER_MAGIC; // using magic number to determin if this file is a legit data base file for our program to read (aka a key to unlock using this program on the file)
  tmp_header->filesize = sizeof(struct db_header_t);

  // set passed-in header's inner pointer to header
  *headerOut = tmp_header; 
  printf("Header appended to new file\n");
  return STATUS_SUCCESS;
}

void output_to_file(int fd, struct db_header_t* headerIn, struct plant_t* plantsIn){
  // first confirm fd is valid
  if (fd < 0){
    printf("Got a bad file descripter from user\n");
    // return STATUS_ERROR;
    return;
  }
  
  // NOTE: We need to store the headerIn->count getting used for loops before it gets converted to network endian
  int count = headerIn->count;

  // reset header data to network endian
  headerIn->magic = htonl(headerIn->magic);
  headerIn->version = htons(headerIn->version);
  headerIn->count = htons(headerIn->count);
  headerIn->filesize = htonl(sizeof(struct db_header_t) + (sizeof(struct plant_t)*count));
  
  // delete file with ftruncate return cursor at end of file back to top in order to overwrite previous header
  ftruncate(fd,0);
  lseek(fd,0, SEEK_SET);
  write(fd,headerIn, sizeof(struct db_header_t));
  
  if(plantsIn){
    for(int i=0; i < count; i++){
      //convert structs to network endian
      //NOTE: There is no need to convert single byte data (aka char*) to network or host endian as a single byte will be interpreted the same by any system
      plantsIn[i].desired_temp = htonl(plantsIn[i].desired_temp);
      plantsIn[i].desired_moist = htonl(plantsIn[i].desired_moist);

      write(fd,&plantsIn[i],sizeof(struct plant_t));
    }
  }
}

int read_plants(int fd, struct db_header_t *headerIn, struct plant_t **plantsOut){
  // first confirm fd is valid
  if (fd < 0){
    printf("Got a bad file descripter from user\n");
    return STATUS_ERROR;
  }

  // guard clause if plants data is empty values to read 
  if(headerIn->count == 0){
  //if(headerIn->count == 0 && headerIn->filesize == sizeof(struct db_header_t){
    return STATUS_SUCCESS;
  }

  int count = headerIn->count;
  
  // create buffer to read into
  *plantsOut = (struct plant_t*)calloc(count,sizeof(struct plant_t));
  if (*plantsOut == NULL){
    perror("read_plants : calloc");
    return STATUS_ERROR;
  }
  read(fd,*plantsOut, count * sizeof(struct plant_t));

  // convert int values to host endian  
  for(int i = 0; i<count; i++){
  // NOTE: Do not need to convert single byte data (i.e. char) into network or host endian as single byte data will always be interpreted the same way on any system
    (*plantsOut)[i].desired_temp = ntohl((*plantsOut)[i].desired_temp);
    (*plantsOut)[i].desired_moist = ntohl((*plantsOut)[i].desired_moist);
  }

  return STATUS_SUCCESS;
}


int add_plant(struct db_header_t* headerIn, struct plant_t** plantsOut, char* addstring){
  char *name = strsep(&addstring,",");
  char *temp = strsep(&addstring,",");
  char *moist = strsep(&addstring,",");
  
  // create tmp count for realloc
  int count = headerIn->count + 1;
  
  struct plant_t* tmp_plants = (struct plant_t*)realloc(*plantsOut, count*(sizeof(struct plant_t)));
  if(tmp_plants == NULL){
    perror("add_plant : realloc");
    return STATUS_ERROR;
  }
  *plantsOut = tmp_plants;

  // NOTE: wrapping double pointer in its own dereference (*plantsOut) then allows for [i] access on the inner pointer
  strncpy((*plantsOut)[count-1].name, name, sizeof((*plantsOut)[count-1].name));
  (*plantsOut)[count-1].desired_temp = atoi(temp);
  (*plantsOut)[count-1].desired_moist = atoi(moist);

  // update new count in header
  headerIn->count = count;
  printf("new count durring add_plant = %d\n",headerIn->count);

  return STATUS_SUCCESS;
}

void displayPlants(struct db_header_t *headerIn ,struct plant_t* plantsIn){
  printf("\n###########################\n");
  printf("\t Plants List \n");
  printf("###########################\n");
  for(int i = 0; i < headerIn->count; i++){
    printf("Plant # %d\n",i+1);
    printf("\tName        : %s\n",plantsIn[i].name);
    printf("\tTemperature : %d\n",plantsIn[i].desired_temp);
    printf("\tMoisture    : %d\n",plantsIn[i].desired_moist);
  }
}

int deletePlant_by_name(struct db_header_t *headerIn, struct plant_t** plantsOut, char* name){
  // first check if name even exists in database
  int index_of_name = -1;
  
  for(int i=0; i<headerIn->count; i++){
    if(strcmp((*plantsOut)[i].name, name) == 0){ // strcmp return 0 if 100% equal
      index_of_name = i;
    }
  }

  printf("\n  index =  %d\n", index_of_name);
  printf("headerIn->count = %d\n", headerIn->count);

  if(index_of_name == -1){
    printf("Plant by name of %s does not exists in database\n",name);
    return STATUS_ERROR;
  }

  // if name exists
  struct plant_t *tmp_plants = (struct plant_t *)malloc(sizeof(struct plant_t)*(headerIn->count-1));
  if(tmp_plants == NULL){
    perror("deletePlant_by_name : malloc");
    return STATUS_ERROR;
  }

  // used to keep track of where we need to skip the copying of
  int j = 0;
  for(int i=0; i < headerIn->count - 1; i++){
    if(i == index_of_name){
      j++; // skip current index of original *plantsOut
    }
    tmp_plants[i] = (*plantsOut)[j];
    j++;
  }

  
  // replace old plants array with new one created using tmp and update count in header 
  headerIn->count --; 
  free(*plantsOut);
  *plantsOut = tmp_plants;
  
  return STATUS_SUCCESS;
}

int updatePlant_moist_by_name(struct db_header_t *headerIn, struct plant_t* plantsOut, char* name_and_newMoist){
  char *name = strsep(&name_and_newMoist,",");
  char *hours = strsep(&name_and_newMoist,","); 
  int newMoist = atoi(hours);

  if(newMoist < 0){
    printf("Moisture cannot be less than 0\n");
    return STATUS_ERROR;
  } 

  // check plants name exists in database
  int index_of_name = -1;
  for(int i=0; i < headerIn->count; i++){
    if(strcmp(plantsOut[i].name,name) == 0){
      index_of_name = i;
      break;
    }
  }

  if(index_of_name == -1){
    printf("Plant by name of %s does not exists in database\n",name);
    return STATUS_ERROR;
  }
  int oldMoist = plantsOut[index_of_name].desired_moist; 
  plantsOut[index_of_name].desired_moist = newMoist;
  printf("Plant \"%s\" moisture updated from %d -> %d\n",name,oldMoist,plantsOut[index_of_name].desired_moist);

  return STATUS_SUCCESS;
}
