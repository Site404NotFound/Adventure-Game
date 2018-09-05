/*********************************************************************
** Author: James Hippler (HipplerJ)
** Oregon State University
** CS 344-400 (Spring 2018)
** Operating Systems I
**
** Description: Program 2 - adventure (Block 2)
** Due: Sunday, May 09, 2018
**
** Filename: hipplerj.buildrooms.c
**
** Objectives:
** This assignment asks you to write a simple game akin to old text adventure
** games like Adventure:
** http://en.wikipedia.org/wiki/Colossal_Cave_Adventure
** You'll write two programs that will introduce you to programming
** in C on UNIX based systems, and will get you familiar with reading
** and writing files.
**
** Overview:
** This assignment is split up into two C programs (no other languages
** is allowed). The first program (hereafter called the "rooms program")
** will be contained in a file named "<STUDENT ONID USERNAME>.buildrooms.c",
** which when compiled with the same name (minus the extension) and
** run creates a series of files that hold descriptions of the in­game rooms
** and how the rooms are connected.
**
** The second program (hereafter called the "game") will be
** called "<STUDENT ONID USERNAME>.adventure.c" and when compiled
** with the same name (minus the extension) and run provides an
** interface for playing the game using the most recently generated rooms.
**
** In the game, the player will begin in the "starting room" and will
** win the game automatically upon entering the "ending room", which
** causes the game to exit, displaying the path taken by the player.
** During the game, the player can also enter a command that returns
** the current time ­ this functionality utilizes mutexes and multithreading.
** For this assignment, do not use the C99 standard: this should
** be done using raw C (which is C89). In the complete example and
** grading instructions below, note the absence of the ­c99 compilation flag.
**
** EXTERNAL RESOURCES:
** - http://www.cplusplus.com/reference/cstdio/sprintf/
** - https://www.geeksforgeeks.org/generating-random-number-range-c/
*********************************************************************/

/************************************************************************
** PREPROCESSOR DIRECTIVES
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>

/************************************************************************
** GLOBAL CONSTANT DECLARATIONS
*************************************************************************/

#define TOTAL_ROOMS 10                                                          // Global Constant for the total list of locations
#define MAX_ROOMS 7                                                             // Global Constant for the maximum number of rooms that can be selected
#define MAX_CONNECTIONS 6                                                       // Global Constant for the maximum number of connections that can be established
#define ROOM_TYPES 2                                                            // Global Constant for the room types (Start and End rooms)
#define DIR_BUFFER 25                                                           // Global Constant for the size of the directory name buffer
#define FILE_BUFFER 35                                                          // Global Constant for the size of the file name buffer

/************************************************************************
** STRUCTURE DECLARATIONS
*************************************************************************/

// Structure to store the room information as they are randomly built
struct room{
  int id;                                                                       // Integer value to store room IDs
  char* name;                                                                   // Character arrays to store room names (strings)
  int numConnections;                                                           // Integer value to store the number of connections established
  char* connections[MAX_CONNECTIONS];                                           // Array of Character arrays to store room connection names
  char* type;                                                                   // Character arrays to store the type of room (start, middle, or end)
};

/************************************************************************
** FUNCTION DECLARATIONS
*************************************************************************/

char* createDirectory();
void randomizeRoomNames(char**, struct room*);
void randomizeRoomTypes(struct room*);
void randomizeRoomConnections(struct room*);
void initializeConnections(struct room*);
bool confirmConnectionSize(struct room*, int);
bool connectionExists(struct room*, int, int);
void addConnection(struct room*, int, int);
void incrementConnections(struct room*, int, int);
int getRoom(struct room*);
bool checkgraph(struct room*);
void randomizer(int*, int, int);
bool checkUnique(int*, int, int);
void writeRoomInformation(char*, struct room*);

/************************************************************************
* Description: main function
* Calls the necessary function to orchestrate the program.  Starts by
* randomizing the room information and then writing that data to
* individual files under a directory with the current PID.
*************************************************************************/

int main(int argc, char const *argv[]){
  srand(time(NULL));                                                            // Seed random number to create random number of connections
  struct room cities[MAX_ROOMS];                                                // Establish a structures array for room that will be passed around and built out
  char *locations[] = {                                                         // Create array of strings to store my locations (they're Morrowind Cities)
    "Caldera",
    "Balmora",
    "DagonFel",
    "Vivec",
    "Gnisis",
    "TelVos",
    "Khuul",
    "MaarGan",
    "Pelagiad",
    "TelMora"
  };

  char* directoryName = createDirectory();                                      // Call function to create a directory where files will be stored
  randomizeRoomNames(locations, cities);                                        // Call function to randomize room names and assigned ids
  randomizeRoomConnections(cities);                                             // Call function to create random connection between each location
  randomizeRoomTypes(cities);                                                   // Call function to randomize room types with only one start and finish
  writeRoomInformation(directoryName, cities);                                  // Call function to write room information to file names in the directory created earlier

  return 0;                                                                     // Exit the program and return to show completion without errors.
}

/************************************************************************
* Description: createDirectory function
* Function is called to create a new directory to store the room files
* that will soon be generated.  The directory name is a combination of
* my username and the current pid (hipplerj.rooms.<PID>).
*************************************************************************/

char* createDirectory(){
  char* dirName = "hipplerj.rooms";                                             // Create a variable with the static portion of the directory name
  int pid = getpid();                                                           // Call the function to get the process id and assign to variable
  static char fullName[DIR_BUFFER];                                             // Create a buffer array to store sprintf data later in the function
  memset(fullName, '\0', DIR_BUFFER);                                           // Clear the buffer array before initializing (safe)
  sprintf(fullName, "%s.%d", dirName, pid);                                     // Use sprintf to combine the static and pid information to a single string
  int result = mkdir(fullName, 0755);                                           // Make a directory with the sprintf buffer information as the name (0755 permissions)
  if(result != 0){                                                              // If the result of creating the directory is not 0
    printf("ERROR: Unable to create Directory %s.%d\n", dirName, pid);          // Throw an error to the console
    exit(1);                                                                    // Exit the program with error status
  }
  return fullName;                                                              // Return the directory Name to main to use later when writing files
}

/************************************************************************
* Description: randomizeRoomNames function
* Function selectes room names at random and assigns their data to the
* room structure.  This ensures that the same rooms are not selected in
* the same order each time the program is run.
*************************************************************************/

void randomizeRoomNames(char** locations, struct room* cities){
  int roomNumbers[MAX_ROOMS];                                                   // Establish an integer array to store random room selections
  int i = 0;
  memset(roomNumbers, '\0', MAX_ROOMS);                                         // Set all elements in the array to null \0
  randomizer(roomNumbers, MAX_ROOMS, TOTAL_ROOMS);                              // Call the randomizer function to select seven rooms at random
  for(i = 0; i < MAX_ROOMS; i ++){
    cities[i].name = locations[roomNumbers[i]];                                 // Work through the random array of room values and create structure names
    cities[i].id = i;                                                           // Work through the random array of room values and create structure IDs.
  }
}

/************************************************************************
* Description: randomizeRoomConnections function
* Function randomizes the connections that each room has established
* in the game.  It follows the rules that a room cannot be connected to
* itself, a room cannot have duplicate connections, and a room must have
* at most 6 and at least 3 connections.
*************************************************************************/

void randomizeRoomConnections(struct room* cities){
  int room1,
      room2;
  initializeConnections(cities);                                                // Call function to initialize connection information to zeros and null
  do{
    room1 = getRoom(cities);                                                    // Randomly select the initial room
    do{
      room2 = getRoom(cities);                                                  // Ranomly select the secondary room
    }while((room1 == room2) || (connectionExists(cities, room1, room2) == true));
    addConnection(cities, room1, room2);                                        // Call function to add the connection between the two random numbers
    incrementConnections(cities, room1, room2);                                 // Increment the number of connections for both randomly selected rooms
  } while(checkgraph(cities) == false);                                         // Continue generating connections until all rooms have at least three.
}

/************************************************************************
* Description: initializeNumConnections function
* Function initializes the total connections for each room to zero
*************************************************************************/

void initializeConnections(struct room* cities){
  int i = 0;
  for(i = 0; i < MAX_ROOMS; i ++){
    cities[i].numConnections = 0;                                               // Initialize the number of connection to zero for all cities
    memset(cities[i].connections, '\0', MAX_ROOMS);                             // Initialize the list of connections to zero for all cities
  }
}

/************************************************************************
* Description: getConnections function
* Function gets a room number at random.  Continues to select rooms until
* it finds one that is not at the maximum number (6) of connections.
*************************************************************************/

int getRoom(struct room* cities){
  int random = 0;
  do{
    random = (rand() % MAX_ROOMS);                                              // Select a room number at random
  }while(confirmConnectionSize(cities, random) != false);                       // Continue selecting until a room is found that is below the maximum connections
  return random;
}

/************************************************************************
* Description: confirmConnectionSize function
* Function confirms that a room has not surpassed its maximum of
* six unique conntections.
*************************************************************************/

bool confirmConnectionSize(struct room* cities, int random){
  if(cities[random].numConnections < MAX_CONNECTIONS){                          // Confirm that the room does not have more than six connections
    return false;
  }
  return true;
}

/************************************************************************
* Description: connectionExists function
* Function confirms that the connection being established randomly
* does not already exist for either room.  I think I really only
* needed to check one room since if a connection exists in one it
* should exist in both based on the rules of our game.
*************************************************************************/

bool connectionExists(struct room* cities, int room1, int room2){
  int i = 0;
  bool exists = false;
  for(i = 0; i < cities[room1].numConnections; i ++){
    if (strcmp(cities[room1].connections[i], cities[room2].name) == 0){         // Check if the connection already exists in the first room
      exists = true;                                                            // If it exists then pick new rooms to attempt to connect
    }
  }
  for(i = 0; i < cities[room2].numConnections; i ++){
    if (strcmp(cities[room2].connections[i], cities[room1].name) == 0){         // Check if the connection already exists in the second room
      exists = true;                                                            // If it exists then pick new rooms to attempt to connect
    }
  }
  return exists;
}

/************************************************************************
* Description: addConnection function
* Add room1 as a connection to room2 and vice versa.  By the time we
* reach this function, it has already been confirmed that the rooms are
* below the maximum and that the connections are unique.
*************************************************************************/

void addConnection(struct room* cities, int room1, int room2){
  cities[room1].connections[cities[room1].numConnections] = cities[room2].name; // Add the name of the second room as a connection to the first room.
  cities[room2].connections[cities[room2].numConnections] = cities[room1].name; // Add the name of the first room as a connection to the second room.
}

/************************************************************************
* Description: incrementConnections function
* Function is called to incremement the total number of connections
* for both rooms that were just connected together.
*************************************************************************/

void incrementConnections(struct room* cities, int room1, int room2){
  cities[room1].numConnections ++;                                              // Increment the first room's number of connections by one
  cities[room2].numConnections ++;                                              // Increment the second room's number of connections by one
}

/************************************************************************
* Description: randomizeRoomTypes function
* Function is called to check if all rooms are at a minimum of three
* connections.  If not, then the program continues to randomly generate
* additional connections.
*************************************************************************/

bool checkgraph(struct room* cities){
  int i = 0;
  for(i = 0; i < 7; i ++){
    if(cities[i].numConnections < 3){                                           // Check all cities to see if their number of connections are at less than three
      return false;                                                             // If there is a room below the minimum return false and keep assigning connections
    }
  }
  return true;
}

/************************************************************************
* Description: randomizeRoomTypes function
* Function randomizes the room types for each array in the structure.
* It starts by making everything a MID_ROOM and then randomly selects
* two rooms to be either the START_ROOM or the END_ROOM.
*************************************************************************/

void randomizeRoomTypes(struct room* cities){
  int i = 0;
  int roomNumbers[ROOM_TYPES];
  memset(roomNumbers, '\0', ROOM_TYPES);
  for(i = 0; i < MAX_ROOMS; i ++){
    cities[i].type = "MID_ROOM";                                                // Initially assign all rooms as MID_ROOM
  }
  randomizer(roomNumbers, ROOM_TYPES, MAX_ROOMS);                               // Call randomizer to pick two numbers at random
  for(i = 0; i < ROOM_TYPES; i ++){
    if(i == 0){
      cities[roomNumbers[i]].type = "START_ROOM";                               // Assign the room associated with the first random number as a START_ROOM
    } else {
      cities[roomNumbers[i]].type = "END_ROOM";                                 // Assign the room associated with the second random number as END_ROOM
    }
  }
}

/************************************************************************
* Description: randomizer function
* Function takes a list of integers and randomizes them.  This is useded
* when randomly selecting our rooms at the outset of the program.
*************************************************************************/

void randomizer(int* randomlist, int arraySize, int range){
  int count = 0,
      random = 0;
  while(count < arraySize){
    random = (rand() % range);                                                  // Grab a random number between 0 and 9
    if(checkUnique(randomlist, random, range) == true){                         // Check if that number is unique by calling a function.
      randomlist[count] = random;                                               // If it's unique, then add it to the integer array
      count ++;                                                                 // Move to the next element in the array
    }
  }
}


/************************************************************************
* Description: checkUnique function
* Function confirms that the random number that is selected is
* unique in a character array.  Confirms that the same room is not
* selected multiple times for use in the program.
*************************************************************************/

bool checkUnique(int* randomlist, int random, int range){
  int i = 0;
  for(i = 0; i < range; i++){
    if(random == randomlist[i]){                                                // If the number already exits in the list
      return false;                                                             // Return false and select another number at random
    }
  }
  return true;
}

/************************************************************************
* Description: writeRoomInformation function
* Function receives the directory name that was created earlier
* and the array of room structures.  Function iterates through each
* room structure and writes that information to individual files.
*************************************************************************/

void writeRoomInformation(char * directory, struct room* cities){
  FILE * f;                                                                     // Create a file pointer to open files for each room
  static char fullName[FILE_BUFFER];                                            // Create a character array to store the full file name that will be created later
  int i = 0,                                                                    // Create a counter variable for the outer for loop
      j = 0;                                                                    // Create a counter variable for the inner for loop
  for(i = 0; i < MAX_ROOMS; i ++){                                              // Iterate through each room structure in the structure array
    memset(fullName, '\0', FILE_BUFFER);                                        // Reset the memory for the fullName array to null values \0
    sprintf(fullName, "%s/%s_room", directory, cities[i].name);                 // Combine the file name with the directory name to create the full save location
    f = fopen(fullName, "a");                                                   // Open the current file to write the current room information
    if(f < 0){                                                                  // If the file cannot be opened for writing (appending)
      fprintf(stderr, "Could not open %s for writing\n", fullName);             // Print and error message to the terminal
      perror("Error in writeRoomInformation()");                                // Also print error message to stderr
      exit(1);                                                                  // Exit the program with a value of 1 (signifying error occurred)
    }
    fprintf(f, "ROOM NAME: %s\n", cities[i].name);                              // Write the room names to the file
    for(j = 0; j < cities[i].numConnections; j ++){                             // Iterate through each rooms connections
      fprintf(f, "CONNECTION %d: %s\n", (j + 1), cities[i].connections[j]);     // Write connection to the file
    }
    fprintf(f, "ROOM TYPE: %s\n", cities[i].type);                              // Write the room type to the file
    fclose(f);                                                                  // Close the file for the room
  }
}
