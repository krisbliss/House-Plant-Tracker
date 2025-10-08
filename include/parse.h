#pragma once

#define HEADER_MAGIC 0x4b424442

struct db_header_t{
  unsigned int magic;
  unsigned short version;
  unsigned short count;
  unsigned int filesize;
};

struct plant_t{
  char name [256];
  unsigned int desired_temp;
  unsigned int desired_moist;
};

int create_db_header(int fd, struct db_header_t **headerOut);
int validate_db_header(int fd, struct db_header_t **headerOut);
int read_plants(int fd, struct db_header_t *headerIn, struct plant_t **plantsOut);
int add_plant(struct db_header_t* headerIn, struct plant_t** plantsOut, char* addstring);
void output_to_file(int fd, struct db_header_t* headerIn, struct plant_t* plantsIn);
void displayPlants(struct db_header_t *headerIn ,struct plant_t* plantsIn);
int deletePlant_by_name(struct db_header_t *headerIn, struct plant_t** plantsOut, char* name);
int updatePlant_moist_by_name(struct db_header_t *headerIn, struct plant_t* plantsOut, char* name_and_newMoist);
