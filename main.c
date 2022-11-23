#include "main.h"

/* 
*  For Checkpoint 2, need to have test plan outline + description of tests
*  Need to have data structure to represent tag array of cache 
*  Need to derive index and tag fields from address
*  Demonstrate functions to implement replacement algorithm
*/

int main(int argc, char *argv[]) {
    char *filename;

    // Need to have filename and mode select
    if(argc != 3) {
        printf("Two arguments required!");
        exit(-1);
    }

    // Determines what mode parses filename
    while(*++argv) {
        if((*argv)[0] == '-') {
            switch((*argv)[1]) {
                
                case 's':
                    printf("Silent Mode Enabled...\n");
                    silentMode = true;
                    break;
                case 'n':
                    printf("Nornal Mode Enabled...\n");
                    break;
                default:
                    printf("Unknonw option -%c\n", (*argv)[1]);
                    exit(-1);
            }
        }
        else {
            filename = (*argv);
        }
    }

    trace = traceParser(filename);
    resetCache();
    for(int i = 0; i < trace.size; i++) {
        switch(trace.command[i]) {
            case(READREQ):
                read(trace.address[i]);
                break;
            case(WRITEREQ):
                break;
            case(READREQI):
                break;
            case(SNOOPINVALD):
                break;
            case(SNOOPREAD):
                break;
            case(SNOOPWRITE):
                break;
            case(SNOOPRWIN):
                break;
            case(CLR):
                break;
            case(PRINT):
                printValid(trace.address[i]);
                break;
            default:
                exit(-1);
        }
        output.cacheRatio = output.cacheHits / (float)(output.cacheReads + output.cacheWrites);
        printOutput();
    }

    return 0;
}

void read(unsigned int address) {
    // dissect address into tag and index
    // check to see if hit
    // if miss, check to see if any line is invalid
    // if line invalid, write to line, set MESI
    // if no invalid, evict line and set MESI
    Derived values;
    int way;
    output.cacheReads++;

    values = addressParser(address);
    printf("\nTag === 0x%x\n", values.tag);
    way = findHit(values);
    
    if (way != -1) {
        // Hit
        output.cacheHits++;
        switch (dCache[values.index].lines[way].MESI) {
            case 'M':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'E':
                dCache[values.index].lines[way].MESI = 'S';
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'S':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'I':
                dCache[values.index].lines[way].MESI = 'S';
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            default:
                exit(-1);
        }
    }
    else {
        output.cacheMiss++;
        way = findEmpty(values.index);
        if (way != -1) {
            // Miss (Empty)
            dCache[values.index].lines[way].MESI = 'E';
            dCache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
        }
        else {
            // Miss (Needs Evict)
            way = getLRU(values.index);
            dCache[values.index].lines[way].MESI = 'E';
            dCache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
        }
    }

}

void write(unsigned int address) {
    // dissect address into tag and index
    // check to see if hit
    // if miss, check to see if any line is invalid
    // if line invalid, write to line, set MESI
    // if no invalid, evict line and set MESI
}

void updateLRU(unsigned int setIndex, int way) {
    unsigned int PLRU = dCache[setIndex].PLRU;

    switch(way) {
        case 0:
            PLRU = PLRU & 0x74;
            break;
        case 1:
            PLRU = (PLRU & 0x74) | 0x8;
            break;
        case 2:
            PLRU = (PLRU & 0x6C) | 0x2;
            break;
        case 3:
            PLRU = (PLRU & 0x6C) | 0x12;
            break;
        case 4:
            PLRU = (PLRU & 0x6A) | 0x1;
            break;
        case 5:
            PLRU = (PLRU & 0x6A) | 0x21;
            break;
        case 6:
            PLRU = (PLRU & 0x3A) | 0x5;
            break;
        case 7:
            PLRU = (PLRU & 0x3A) | 0x45;
            break;
    }
    dCache[setIndex].PLRU = PLRU;
}

int getLRU(unsigned int setIndex) {
    int way;
    unsigned int PLRU = dCache[setIndex].PLRU;

    if( (PLRU & 0x1) == 0 ) {
        if( ((PLRU >> 2) & 0x1) == 0 ) {
            if( ((PLRU >> 3) & 0x1) == 0 ) {
                way = 1;
            }
            else {
                way = 0;
            }
        }
        else {
            if( ((PLRU >> 6) & 0x1) == 0 ) {
                way = 3;
            }
            else {
                way = 2;
            }
        }
    }
    else {
        if( ((PLRU >> 1) & 0x1) == 0 ) {
            if( ((PLRU >> 5) & 0x1) == 0 ) {
                way = 5;
            }
            else {
                way = 4;
            }
        }
        else {
            if( ((PLRU >> 4) & 0x1) == 0 ) {
                way = 7;
            }
            else {
                way = 6;
            }
        }
    }

    return way;
}

int findEmpty(unsigned int setIndex) {
    int way;

    for (int i = 0; i < NUMWAYS; i++) {
        if (dCache[setIndex].lines[i].MESI == 'I') {
            return way = i;
        }
    }
    return -1;
}

int findHit(Derived values) {
    int way;
    for (int i = 0; i < NUMWAYS; i++) {
        if (dCache[values.index].lines[i].tag == values.tag) {
            return way = i;
        }
    }
    return -1;
}

void messageToCache(int Message, unsigned int Address) {
    if(!silentMode) {
        printf("L2: %s %x\n", messageChar[Message], Address);
    }
}

void printValid(unsigned int address) {
    Derived values;
    values = addressParser(address);

    for (int i = 0; i < NUMWAYS; i++) {
        if (dCache[values.index].lines[i].MESI == 'I') {
            continue;
        }
        else {
            printf("\nCache Set   : %d\n",  values.index);
            printf("Line        : %d\n",    i);
            printf("Tag         : 0x%x\n",  dCache[values.index].lines[i].tag);
            printf("MESI        : %c\n\n",  dCache[values.index].lines[i].MESI);
        }
    }
}

void printOutput() {
    printf("\nTotal Cache Reads:    %d\n",    output.cacheReads);
    printf("Total Cache Writes:   %d\n",      output.cacheWrites);
    printf("Total Cache Hits:     %d\n",      output.cacheHits);
    printf("Total Cache Misses:   %d\n",      output.cacheMiss);
    printf("Cache Hit Ratio:      %.3f\n\n",  output.cacheRatio);
}

void resetCache() {
    for (int i = 0; i < NUMSETS; i++) {
        for (int j = 0; j < NUMWAYS; j++) {
            dCache[i].lines[j].MESI = 'I';
            dCache[i].lines[j].tag = 0;
            iCache[i].lines[j].MESI = 'I';
            iCache[i].lines[j].tag = 0;
        }
    }

    output.cacheHits = 0;
    output.cacheMiss = 0;
    output.cacheRatio = 0;
    output.cacheReads = 0;
    output.cacheWrites = 0.0;

}

Derived addressParser(unsigned int address) {
    Derived values;

    values.index = (address >> BYTEOFF) & INDEXMASK;
    values.tag = (address >> (BYTEOFF + INDEXOFF)) & TAGMASK;
    return values;
}

TraceValues traceParser(char *filename) {
    FILE *fp;
    TraceValues trace;
    int i = 0;
    fp = fopen(filename, "r");

    if(fp == NULL) {
        printf("Unable to open file!\n");
        exit(-1);
    }

    if(!silentMode) {
        printf("===Command===|===Address===\n");
    }

    // Reads and parses file into struct
    while(fscanf(fp, "%d %x", &trace.command[i], &trace.address[i]) != EOF) {
        if(!silentMode) {
            printf("      %d          %x\n", trace.command[i], trace.address[i]);
        }
        i++;
    }
    trace.size = i;

    return trace;
}





