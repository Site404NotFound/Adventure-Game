/*********************************************************************
** Author: James Hippler (HipplerJ)
** Oregon State University
** CS 344-400 (Spring 2018)
** Operating Systems I
**
** Description: Program 2 - adventure (Block 2)
** Due: Sunday, May 09, 2018
**
** Filename: hipplerj.adventure.c
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
** EXTERNAL RESOURCES
** - https://stackoverflow.com/questions/27491005/how-can-i-get-a-string-
**   from-input-without-including-a-newline-using-fgets
** - https://www.geeksforgeeks.org/c-program-count-number-lines-file/
** - https://stackoverflow.com/questions/4214314/get-a-substring-of-a-
**   char?utm_medium=organic&utm_source=google_rich_qa&utm_campaign=google_rich_qa
** - http://www.cplusplus.com/reference/ctime/strftime/
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
#include <dirent.h>
#include <pthread.h>
#include <assert.h>

/************************************************************************
** GLOBAL CONSTANT DECLARATIONS
*************************************************************************/

#define TOTAL_FILES 7                                                           // Global Constant for the total number of files that were created
#define MAX_CONNECTIONS 6                                                       // Global Constant for the maximum number of connections that can be established
#define FILE_BUFFER 35                                                          // Global Constant for the size of the file name buffer
#define LINE_BUFFER 50                                                          // Global Constant for the size of the line buffer
#define NAME_BUFFER 10                                                          // Global Constant for the size of the name buffer
#define TYPE_BUFFER 20                                                          // Global Constant for the size of the type buffer
#define PATH_SIZE 1024                                                          // Global Constant for the size of the Path Array
#define DIR_PREFIX "hipplerj.rooms."                                            // Global Constant for the string prefix of the programs directories
#define TIME_FILE "currentTime.txt"                                             // Global Constant for the time file Name that will be created later

/************************************************************************
** STRUCTURE DECLARATIONS
*************************************************************************/

// Rtructure room holds room information read in from the files
struct room{
  char name[NAME_BUFFER];                                                       // Character arrays to store room names (strings)
  int numConnections;                                                           // Integer value to store the number of connections established
  char connections[MAX_CONNECTIONS][NAME_BUFFER];                               // Array of Character arrays to store room connection names
  char type[TYPE_BUFFER];                                                       // Character arrays to store the type of room (start, middle, or end)
};

// structure victory holds the path and steps followed to win the game
struct victory{
  char path[PATH_SIZE][NAME_BUFFER];                                            // Array of Character Arrays stores the paths that were taken to end the game
  int steps;                                                                    // Integer array to store the total number of steps taken
};

struct victory complete;                                                        // Globally initializes the victory structure as complete.
pthread_mutex_t mutexT;                                                         // Declare mutex Threading to be used to show time during the game

/************************************************************************
** FUNCTION DECLARATIONS
*************************************************************************/

char* getCurrentDirectory();
void getRoomFiles(struct room*, char*);
void getLocationData(struct room*, char*, char*, int);
char* createFullPath(char*, char*);
int getTotalLines(FILE* fp, struct room*, int);
void removeBreakLine(char*);
void getRoomName(struct room*, char*, int);
void getConnectionName(struct room*, char*, int, int);
void getRoomType(struct room*, char*, int);
int getStartingRoom(struct room*);
void playGame(struct room*, int);
int evaluateReponse(struct room*, char*, int);
int getNewRoom(struct room*, char*);
bool reachedEnd(struct room*, int);
void* writeTime();
void thread();
void readTime();
void winner();

/************************************************************************
* Description: main function
* Calls the necessary function to orchestrate the program.  Also
* establishes an array of structures that will store our room information
* as it is read from each file.
*************************************************************************/

int main(int argc, char const *argv[]){
  struct room cities[TOTAL_FILES];                                              // Establish a structures array for room that will be passed around and built out
  getRoomFiles(cities, getCurrentDirectory());                                  // Call Function to get the room information.  Send it the name of the most current directory
  playGame(cities, getStartingRoom(cities));                                    // Call function to start playing the game.
  winner();                                                                     // Call function once the game has one and print the results to the user.
  return 0;                                                                     // Exit main function and program with normal status (NO ERRORS).
}

/************************************************************************
* Description: getCurrentDirectory function
* Function opens the current working directory and searches each sub
* directory for the prefix 'hipplerj.rooms.' and collects its name
* and time.  The newest version of the directory is returned to main.
* function receives no input from the main function.  I utilized a lot
* of the code directly from the Block 2: 2.4 Manipula ng Directories
* document.
*
* EXTERNAL RESOURCE:
* https://stackoverflow.com/questions/27491005/how-can-i-get-a-string-
* from-input-without-including-a-newline-using-fgets
*************************************************************************/

char* getCurrentDirectory(){
  int newestDirTime = -1;                                                       // Modified timestamp of newest subdir examined
  char targetDirPrefix[20] = DIR_PREFIX;                                        // Prefix we're looking for
  static char newestDirName[256];                                               // Holds the name of the newest dir that contains prefix
  memset(newestDirName, '\0', sizeof(newestDirName));                           // Set the memory range for the current directory name buffer to null \0
  DIR* current = opendir(".");                                                  // Holds the results from opening the current pwd.
  struct dirent *fileInDir;                                                     // Holds the current subdir of the starting dir
  struct stat dirAttributes;                                                    // Holds information we've gained about subdir

  if(current > 0){                                                              // Make sure the current directory could be opened
    while((fileInDir = readdir(current)) != NULL){                              // Check each entry in dir
      if (strstr(fileInDir -> d_name, targetDirPrefix) != NULL){                // If entry has prefix
        stat(fileInDir -> d_name, &dirAttributes);                              // Get attributes of the entry
        if((int)dirAttributes.st_mtime > newestDirTime){                        // If this time is bigger
          newestDirTime = (int)dirAttributes.st_mtime;
          memset(newestDirName, '\0', sizeof(newestDirName));
          strcpy(newestDirName, fileInDir -> d_name);
        }
      }
    }
  } else {
    fprintf(stderr, "Could not open Present Working Directory\n");              // Print and error message to the terminal
    perror("Error in getCurrentDirectory()");                                   // Also print error message to stderr
    exit(1);                                                                    // Exit the program with a value of 1 (signifying error occurred)
  }
  closedir(current);                                                            // Close the directory we opened
  return newestDirName;
}

/************************************************************************
* Description: getRoomFiles function
* Functions grabs each file from the most current game directory and
* sends that information another function to be processed and stored
* into the rooms structure.
*************************************************************************/

void getRoomFiles(struct room* cities, char* startingDir){
  int count = 0,
      fileNumber = 0;
  DIR * dir = opendir(startingDir);                                             // Create a directory pointer and initialize to the current room directory
  struct dirent *ent;
  if(dir != NULL) {
    while((ent = readdir (dir)) != NULL){
      if(count >= 2){
        getLocationData(cities, ent -> d_name, startingDir, fileNumber);
        fileNumber ++;
      }
      count ++;
    }
    closedir (dir);
  } else {
    fprintf(stderr, "Could not open the %s directory\n", startingDir);          // Print and error message to the terminal
    perror("Error in readRoomLocations()");                                     // Also print error message to stderr
    exit(1);
  }
}

/************************************************************************
* Description: getLocationData function
* Function incremenets through each line of each file and send that
* line to the appropriate function to get values like the room's name,
* the number and names of its connections, and its type.  All information
* is stored to the rooms structure.
*************************************************************************/

void getLocationData(struct room* cities, char* roomFile, char* roomDir, int fileNumber){
  FILE *fp;
  int connections = 0,
      connCount = 0,
      counter = 0;
  char line [LINE_BUFFER];
  memset(line, '\0', LINE_BUFFER);
  fp = fopen((createFullPath(roomFile, roomDir)), "r");
  if(fp > 0){
    connections = (getTotalLines(fp, cities, fileNumber) - 2);
    fseek(fp, 0, SEEK_SET);                                                     // Set the pointer back to the beginning of the file
    while(fgets(line,sizeof(line), fp) != NULL){                                // Loop through each file line by line and store the character array.
      removeBreakLine(line);
      if(counter == 0){                                                         // If it's the first line in the file it's the name line
        getRoomName(cities, line, fileNumber);
      } else if (counter >= 1 && counter < (1 + connections)){                  // If it's not the first line in the file and it's less than the numbner of connections
        getConnectionName(cities, line, fileNumber, connCount);
        connCount ++;
      } else {                                                                  // Otherwise it's the TypeLine
        getRoomType(cities, line, fileNumber);
      }
      counter ++;
    }
  } else {
    fprintf(stderr, "Could not open the \'%s/%s\' file\n", roomDir, roomFile);  // Print and error message to the terminal
    perror("Error in getLocationData()");                                       // Also print error message to stderr
    exit(1);                                                                    // Exit the program with an error value
  }
  fclose(fp);                                                                   // Close the current file
}

/************************************************************************
* Description: createFullPath function
* Function takes the current directory information and the list of
* room files (sent one at a time) and combines that information into
* one single string containing the full path.
*************************************************************************/

char* createFullPath(char* roomFile, char* roomDir){
  static char fullName[FILE_BUFFER];                                            // Create a character array to store the full file name that will be created later
  memset(fullName, '\0', FILE_BUFFER);                                          // Reset the memory for the fullName array to null values \0
  sprintf(fullName, "%s/%s", roomDir, roomFile);                                // Combine the file name with the directory name to create the full save location
  return fullName;                                                              // Return the full fill path name back to calling function
}

/************************************************************************
* Description: getTotalLines function
* Function receives a file pointer as an argument and counts the
* number of lines present in the file.  This is mostly useful for use
* to determine the number of connections that must be processed per
* each file that is read.
*
* EXTERNAL RESOURCES:
* - https://www.geeksforgeeks.org/c-program-count-number-lines-file/
*************************************************************************/

int getTotalLines(FILE* fp, struct room* cities, int fileNumber){
  char c;                                                                       // Character value to store each element in the file
  int count = 0;                                                                // Set a counter.  This will store our line count
  for(c = getc(fp); c != EOF; c = getc(fp)){                                    // Iterate through the entire file, character by character
    if(c == '\n'){                                                              // If a break line is encountered
      count = count + 1;                                                        // Increment the line count counter
    }
  }
  cities[fileNumber].numConnections = (count - 2);                              // Assign the line count - 2 as the number of connections for each cities structure
  return count;                                                                 // Return the count integer to the calling function
}

/************************************************************************
* Description: removeBreakLine function
* Function receives a string as an argument and replaces trailing
* break line characters with a null terminator.
*************************************************************************/

void removeBreakLine(char* line){
  line[strcspn(line, "\n")] = '\0';                                             // Replace the break line with a null terminator

}

/************************************************************************
* Description: getRoomName function
* Function receives the line with the room name and processses that
* information into the room structure array in the appropriate places
*************************************************************************/

void getRoomName(struct room* cities, char* line, int fileNumber){
  char roomName[10] = { 0 };                                                    // Create a character array that will store the parsesed line information
  memset(roomName, '\0', 10);                                                   // Format the new character array to all null terminators
  memcpy(roomName, &line[11], strlen(line) - 11);                               // Skip the Detail information and grab from the beginning of the room name to the end of the string
  strncpy(cities[fileNumber].name, roomName, strlen(roomName) + 1);             // Copy that local char array to the name section of the cities structure
}

/************************************************************************
* Description: getConnectionName function
* Function receives the file lines containing the connection information
* and parses that information into the rooms structure for the game to
* use when presenting optional paths
*************************************************************************/

void getConnectionName(struct room* cities, char* line, int fileNumber, int conn){
  char connection[10] = { 0 };                                                  // Create a character array that will store the parsesed line information
  memset(connection, '\0', 10);                                                 // Format the new character array to all null terminators
  memcpy(connection, &line[14], strlen(line) - 13);                             // Skip the Detail information and grab from the beginning of the room connection to the end of the string
  strncpy(cities[fileNumber].connections[conn], connection, strlen(connection) + 1);
  // Copy that local char array to the connection section of the cities structure
}

/************************************************************************
* Description: getRoomType function
* Function receives the file lines containing the room type information
* and parses that information into the room structure for later
* utilization in the game function.
*************************************************************************/

void getRoomType(struct room* cities, char* line, int fileNumber){
  char roomType[20] = { 0 };                                                    // Create a character array that will store the parsesed line information
  memset(roomType, '\0', 20);                                                   // Format the new character array to all null terminators
  memcpy(roomType, &line[11], strlen(line) - 11);                               // Skip the Detail information and grab from the beginning of the room type to the end of the string
  strncpy(cities[fileNumber].type, roomType, strlen(roomType) + 1);             // Copy that local char array to the type section of the cities structure
}

/************************************************************************
* Description: getStartingRoom function
* Determines that starting by searching each room type for the
* keyword START_ROOM.  Returns the room number (struct array element)
* to main so that the game may begin
*************************************************************************/

int getStartingRoom(struct room* cities){
  int i = 0,                                                                    // Establish counter variable to iterate through loop
      roomNumber = 0;                                                           // Integer variable to store the starting room number
  for(i = 0; i < TOTAL_FILES; i ++){                                            // Iterate through the entire structure array room types
    if (strcmp(cities[i].type, "START_ROOM") == 0){                             // If the room type is START_ROOM
      roomNumber = i;                                                           // Assign that number to the room number
    }
  }
  return roomNumber;                                                            // Return the starting room number to main.
}

/************************************************************************
* Description: playGame function
* This is were most of the game functionality occurs.  The function
* gets and displays the current room information to the user and then
* askes for their input.  Input is evaluated and the game continues
* until the user selects the exit room.  Also, if the user inputs the
* word "time", a secondary thread is created to write the time
* information to a file, then the main thread reads the file and outputs
* the result to the console.
*************************************************************************/

void playGame(struct room* cities, int roomNumber){
  complete.steps = 0;                                                           // Set the total number of steps taken to 0
  int i = 0;                                                                    // Establish counter variable to increment through room connections
  bool wantTime = false;
  char* response = NULL;
  size_t bufferSize = 0;
  do{
    printf("CURRENT LOCATION: %s\n", cities[roomNumber].name);                  // Display the current location name
    printf("POSSIBLE CONNECTIONS: ");                                           // Display the current possible connections for the current location
    for(i = 0; i < cities[roomNumber].numConnections; i ++){
      printf("%s", cities[roomNumber].connections[i]);
      if(i != (cities[roomNumber].numConnections) -1) {
        printf(", ");
      } else {
        printf(".\n");
      }
    }
    do{
      printf("WHERE TO? >");                                                    // Prompt the user to either select time or a connection
      getline(&response, &bufferSize, stdin);                                   // Use getline function to store the user input into a character array
      removeBreakLine(response);                                                // Call function to remove the trailing break line
      if(strcmp(response, "time") == 0){                                        // If user inputs "time" into the prompt
        thread();                                                               // Call the function to create a new mutex thread
        readTime();                                                             // Call function to print the current time to the screen
        wantTime = true;
      } else {
        wantTime = false;
      }
    }while(wantTime == true);
    roomNumber = evaluateReponse(cities, response, roomNumber);                 // If input is anything other than time, call the function to evaluate
  }while(reachedEnd(cities, roomNumber) == false);
}

/************************************************************************
* Description: evaluateReponse function
* Function takes the users response (after time has already been evaluated)
* and determines if it's either a valid connection or something that
* cannot be understood by the game.  If it's a valid connection then it
* returns the roomNumber for the new room.
*************************************************************************/

int evaluateReponse(struct room* cities, char* response, int roomNumber){
  int i = 0;                                                                    // Integer for incrementing through all location information.
  bool connection = false;                                                      // Boolean variable to determine if value entered is an existing connection
  for(i = 0; i < cities[roomNumber].numConnections; i ++){                      // Loop through the connections listed for the file
    if(strcmp(response, cities[roomNumber].connections[i]) == 0){               // If the response matches one of those connections
      roomNumber = getNewRoom(cities, cities[roomNumber].connections[i]);       // Set the roomNumber equivalent to the new room
      connection = true;                                                        // Set the boolean variable to true, indicating the requested connection is present
      strncpy(complete.path[complete.steps], response, strlen(response) + 1);   // Add the new room to the path to victory
      complete.steps ++;                                                        // Increment the number of steps the user has taken
      connection = true;
    }
  }
  if (connection == false){
    printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN.\n");                // If user input a room connection that does not exist
  }
  printf("\n");                                                                 // Print an extra break line for good measure
  return roomNumber;                                                            // Return the updated room information back to the calling function
}

/************************************************************************
* Description: getNewRoom function
* Function is called to get the room Number and information for the
* connection that was requested by the user.
*************************************************************************/

int getNewRoom(struct room* cities, char* roomName){
  int i = 0,
      roomNumber = 0;
  for(i = 0; i < TOTAL_FILES; i ++){
    if(strcmp(roomName, cities[i].name) == 0){                                  // Find the room information for the connection that was input by the user
      roomNumber = i;                                                           // Assign that information to the roomNumber integer variable
    }
  }
  return roomNumber;                                                            // Return the roomNumber integer variable to game so it can pull that data
}

/************************************************************************
* Description: reachedEnd function
* Function determines if the user has reached a room with a Type of
* END_ROOM.  It returns a boolean true of END_ROOM is found or False if
* any other result.
*************************************************************************/

bool reachedEnd(struct room* cities, int roomNumber){
  if(strcmp(cities[roomNumber].type, "END_ROOM") == 0){                         // See if the current room is the END_ROOM
    return true;                                                                // If it is, return true to alert the game to end
  }
  return false;                                                                 // Otherwise return false
}

/************************************************************************
* Description: readTime function
* Function opens the currentTime.txt file and reads the time information
* this is called from within the game function and is run within the
* original main thread.
*************************************************************************/

void readTime(){
    FILE * f;                                                                   // Create a file pointer
    f = fopen(TIME_FILE,"r");                                                   // Open that currentTime.txt file for reading and assign to file pointer
    char buffer[LINE_BUFFER];                                                   // Create a character array for storing time information
    memset(buffer,'\0',sizeof(buffer));                                         // Initialize and clear the buffer
    if(f < 0){                                                                  // If the file cannot be opened for writing (appending)
      fprintf(stderr, "Could not open %s for writing\n", TIME_FILE);            // Print and error message to the terminal
      perror("Error in readTime()");                                            // Also print error message to stderr
      return;                                                                   // Return to the calling function after printing error
    }
    fgets(buffer, LINE_BUFFER, f);                                              // Grab the time information from the currentTime.txt file
    printf("\n%s\n", buffer);                                                   // Print the time to the console
    fclose(f);                                                                  // Close the currentTime.txt file
}

/************************************************************************
* Description: writeTime function
* Get the current local time and write that information to a file called
* currentTime.txt that will be read later.  This function is called
* within its own thread when time is input in the game function.
*************************************************************************/

void* writeTime(){
    FILE * f;                                                                   // Create a file pointer
    f = fopen(TIME_FILE, "w");                                                  // Open the currentTime.txt file for writing
    char buffer[LINE_BUFFER];                                                   // Create a buffer to store the time information before writing
    memset(buffer, '\0', LINE_BUFFER);                                          // Clear the buffer and set all elements to a null terminator
    struct tm * t;
    time_t current = time (0);
    t = localtime (&current);                                                   // Set the current local time
    strftime(buffer, sizeof(buffer), "%I:%M%P, %A, %B %d, %Y", t);              // Get the time information (in the appropriate format) and assigned to char array.
    if(f < 0){                                                                  // If the file cannot be opened for writing (appending)
      fprintf(stderr, "Could not open %s for writing\n", TIME_FILE);            // Print and error message to the terminal
      perror("Error in writeTime()");                                           // Also print error message to stderr
      exit(1);                                                                  // Exit the program with a value of 1 (signifying error occurred)
    }
    fprintf(f, "%s\n", buffer);                                                 // Write the time information to the file
    fclose(f);                                                                  // Close the file
    return NULL;                                                                // Return NULL to satisfy the void* function declaration
}

/************************************************************************
* Description: thread function
* Function deals with mutexing and manages our threads when the time
* command is invoked from the game function.
*************************************************************************/

void thread(){
    pthread_t thread;
    pthread_mutex_lock(&mutexT);                                                // Perform lock on the mutex thread
    pthread_create(&thread, NULL, writeTime, NULL);                             // Create new thread that writes the time to a file.
    pthread_mutex_unlock(&mutexT);                                              // A thread unlocks a mutex variable that it has previously locked like so
    pthread_mutex_destroy(&mutexT);                                             // Terminate thread that was created
    pthread_join(thread, NULL);                                                 // Wait for thread to terminate (I honestly don't know which of these is necessary)
}

/************************************************************************
* Description: winner function
* Function outputs message to the screen that the user has successfully
* found the END_ROOM and has won the game.  Also displays the amount of
* steps taken and the path that was followed. Receives nothing from
* main.  Returns nothing to main
*************************************************************************/

void winner(){
  int i = 0;
  printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!\n");                    // Let the user know they actually won.
  printf("YOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", complete.steps);     // Print the number of steps...
  for(i = 0; i < complete.steps; i ++){                                         // Increment through each step taken.
    printf("%s\n", complete.path[i]);                                           // ...and the location names along the way.
  }
}
