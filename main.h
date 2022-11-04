#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BUFFER 100



// Global flag to set mode
bool silentModeGbl;

// Struct to hold values from trace
typedef struct {
    unsigned int command[BUFFER];
    unsigned int address[BUFFER];
} traceValues;

// Fucntion prototypes
traceValues traceParser(char *filename);