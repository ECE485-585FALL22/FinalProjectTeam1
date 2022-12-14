#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

/*----------------------------------
              Defines           
----------------------------------*/
#define BUFFER      100
#define INDEXMASK   0x00007FFF
#define TAGMASK     0x000007FF
#define BYTEMASK    0x00000003
#define BYTEOFF     6
#define INDEXOFF    15
#define NUMWAYS     8
#define NUMSETS     32768

/*----------------------------------
               Enums            
----------------------------------*/
/* Bus Operation types */
enum BusOp {
    READ,
    WRITE,
    INVALIDATE,
    RWIM
};

/* Snoop Result types/Testing only  
enum SnoopOp {
    NOHIT,
    HIT,
    HITM
};
*/

/* Snoop Result types */
enum SnoopOp {
    HIT,
    HITM,
    NOHIT
};

/* L2 to L1 message types */
enum MessageType {
    GETLINE,
    SENDLINE,
    INVALIDATELINE,
    EVICTLINE
};

enum Commands {
    READREQ,
    WRITEREQ,
    READREQI,
    SNOOPINVALD,
    SNOOPREAD,
    SNOOPWRITE,
    SNOOPRWIN,
    TEST,
    CLR,
    PRINT
};

/*----------------------------------
            Char Arrays             
----------------------------------*/
const char *busChar[4] = {
    "READ",
    "WRITE",
    "INVALIDATE",
    "RWIM"
};

/* For testing only
const char *snoopChar[3] = {
    "NOHIT",
    "HIT",
    "HITM"
};
*/

const char* snoopChar[3] = {
    "HIT",
    "HITM",
    "NOHIT"
};

const char *messageChar[4] = {
    "GETLINE",
    "SENDLINE",
    "INVALIDATELINE",
    "EVICTLINE"
};

/*----------------------------------
          Typedef Structs              
----------------------------------*/
typedef struct {
    unsigned int command[BUFFER];
    unsigned int address[BUFFER];
    unsigned int size;
} TraceValues;

typedef struct {
    unsigned int cacheReads;
    unsigned int cacheWrites;
    unsigned int cacheHits;
    unsigned int cacheMiss;
    float        cacheRatio;
} CacheOutput;

typedef struct {
    char         MESI;
    unsigned int tag;
} Lines;

typedef struct {
    unsigned int PLRU;
    Lines lines[NUMWAYS];
} Set;

typedef struct {
    unsigned int index;
    unsigned int tag;
} Derived;

/*----------------------------------
              Global           
----------------------------------*/
bool silentMode;
TraceValues trace;
CacheOutput output = {0};
Set cache[NUMSETS];

/*----------------------------------
         Fucntion prototypes             
----------------------------------*/
// Parse functions
TraceValues traceParser(char *filename);
Derived addressParser(unsigned int address);

// LRU functions
void updateLRU(unsigned int setIndex, int way);
int getLRU(unsigned int setIndex);

// Set checking functions
int findEmpty(unsigned int setIndex);
int findHit(Derived values);

// Snooping functions
void snoopInvaild(unsigned int address);
void snoopRead(unsigned int address);
void snoopWrite(unsigned int address);
void snoopReadM(unsigned int address);

// Read/Write functions
void read(unsigned int address);
void write(unsigned int address);

// Helper functions
void resetCache();
void writeMESI(unsigned int setIndex, int way, int *snoopResult);

// Printing functions
void busOperation(int BusOp, unsigned int Address, int *SnoopResult);
void putSnoopResult(unsigned int Address, int SnoopResult);
void messageToCache(int Message, unsigned int Address);
int getSnoopResult(unsigned int Address);
void printOutput();
void printValid(unsigned int address);


