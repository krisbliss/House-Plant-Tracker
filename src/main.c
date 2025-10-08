#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char* argv[]){
  printf("Usage: %s -f <database_file>\n",argv[0]);
  printf("\t -f : (required) <path_to_database_file> \n");
  printf("\t -n : create new database file (i.e. -nf mydb.db)\n");
  printf("\t -l : print out database (i.e. -lf mydb.db)\n");
  printf("\t -a : <name_of_plant,desired_temp_F,desired_moist_%%> adds a plant to the end of the database (i.e. -a Monstera,75,70) \n");
  printf("\t -d : <name_of_plant_to_delete> deletes plant and it's data from database (i.e. -d Monstera)\n");
  printf("\t -u : <name_of_plant,new_desired_moist_%%> update desired desired moisture %% listed in database of a plant (i.e. -u Monstera,80)\n");
  printf("\t -h : prints this help message\n");
}

int main (int argc, char* argv[]){

  bool printHelp = false; 
  char* filepath = NULL;
  bool newFile = false;
  bool listPlants = false;
  int db_fd = -1;
  char* addstring = NULL;
  char* deletePlant = NULL;
  char* name_and_newMoist = NULL;
  struct db_header_t *header = NULL;
  struct plant_t *plants = NULL;

  int c;
  // loop to read through all cmd args, c = -1 when parser no longer detects any flags 
  while( (c = getopt(argc,argv,"nlhf:a:d:u:")) != -1 ){
    
    // using switch case to modularize handeling 
    switch (c) { 
        case 'n':
          newFile = true;
          break;

        case 'f':
          filepath = optarg; // from getopt, pointer to cmd arg after -f 
          break;
        
        case 'a':
          addstring = optarg;
          break;

        case 'l':
          listPlants = true;
          break;
        
        case 'd':
           deletePlant = optarg;
          break;

        case 'u':
          name_and_newMoist = optarg;
          break;

        case 'h':
          printHelp = true;
          break;
         
        case '?':
          printf("Unknown option -%c\n",c);
          printHelp = true;
          break;

        default:
          print_usage(argv);
          return -1;
    }
  }

  if(printHelp){
    print_usage(argv);
    return 0;
  }

  // program cannot continue without a file path argument
  if (filepath == NULL){
    printf("Filepath is a required argument.\n");
    print_usage(argv);
    return 0;
  }

  // create file if path to file in arg does not exsist
  if (newFile){
    db_fd = create_db_file(filepath);
    if (db_fd == STATUS_ERROR){
      printf("Unable to create database file. Exiting program\n");
      return -1;
    }

    // create header for new file
    if (create_db_header(db_fd, &header) == STATUS_ERROR){
      printf("Could not append header to newly created file. Exiting program\n");
      return -1;
    }
  }
 
  // file already exsists
  else{
    db_fd = open_db_file(filepath);
    if(db_fd == STATUS_ERROR){
      printf("Unable to open database file. Exiting program\n");
      return -1;
    }

    // validate header (if header is valid, this step also ensures that the data in header are unpacked into host endian) 
    if(validate_db_header(db_fd,&header) == STATUS_ERROR){
      printf("Validation failed. Exiting program\n");
      return -1;
    }
  }
  
  // read from data base the plants
  if(read_plants(db_fd,header,&plants) == STATUS_ERROR){
    printf("Failed to read plants from data base. Exiting program\n");
    return -1;
  }

  if (addstring){
    if(add_plant(header, &plants, addstring) == STATUS_ERROR){
      printf("Failed to add plant. Exiting program\n");
      return -1;
    }

    printf("header->count after add : %d\n",header->count);
  }

  if(name_and_newMoist){
    if(updatePlant_moist_by_name(header,plants,name_and_newMoist) == STATUS_ERROR){
      printf("Failed to update the desired moisture of given plant. Exiting program\n");
      return -1;
    }
  }

  if(deletePlant){
    if(deletePlant_by_name(header, &plants, deletePlant) == STATUS_ERROR){
      printf("Could not preform delete operation. Exiting program\n");
      return -1;
    }
  }

  if (listPlants){
     displayPlants(header,plants);
  }
  
  // overwrite db file with updated header and employee data and convert output to network endian
  output_to_file(db_fd, header, plants);

  printf("New file: %d\n",newFile);
  printf("Filepath: %s\n",filepath);

  // close file if its has not been already
  if(db_fd != -1) close(db_fd);

  return 0;
}
