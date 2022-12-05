#include "main.h"

/*----------------------------------
                INDEX
----------------------------------*/


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
                    printf("\nSILENT MODE ENABLED\n");
                    silentMode = true;
                    break;
                case 'n':
                    printf("\nNORMAL MODE ENABLED\n");
                    break;
                default:
                    printf("UNKNOWN OPTION -%c\n", (*argv)[1]);
                    exit(-1);
            }
        }
        else {
            filename = (*argv);
        }
    }

    // Parse trace & reset cache
    trace = traceParser(filename);
    resetCache();


    // Main Loop
    for(int i = 0; i < trace.size; i++) {
        switch(trace.command[i]) {
            case(READREQ):
                read(trace.address[i]);
                break;
            case(WRITEREQ):
                write(trace.address[i]);
                break;
            case(READREQI):
                read(trace.address[i]);
                break;
            case(SNOOPINVALD):
                snoopInvaild(trace.address[i]);
                break;
            case(SNOOPREAD):
                snoopRead(trace.address[i]);
                break;
            case(SNOOPWRITE):
                snoopWrite(trace.address[i]);
                break;
            case(SNOOPRWIN):
                snoopReadM(trace.address[i]);
                break;
            case(CLR):
                resetCache();
                break;
            case(PRINT):
                printValid(trace.address[i]);
                break;
            default:
                exit(-1);
        }
    }
    // Output statistics
    output.cacheRatio = output.cacheHits / (float)(output.cacheReads + output.cacheWrites);
    printOutput();
    return 0;
}

void read(unsigned int address) {
    Derived values;
    int way;
    int snoopResult;
    output.cacheReads++;

    // Parse address & check for hit in set
    values = addressParser(address);
    way = findHit(values);
    
    if (way != -1) {
        // Hit, update LRU and send message to L1
        output.cacheHits++;
        switch (cache[values.index].lines[way].MESI) {
            case 'M':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'E':
                updateLRU(values.index, way);
                messageToCache(SENDLINE, address);
                break;
            case 'S':
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
            // Read from memory to line, set tag & MESI
            // send message to L1
            busOperation(READ, address, &snoopResult);
            writeMESI(values.index, way, &snoopResult);
            cache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
            messageToCache(SENDLINE, address);
        }
        else {
            // Miss (Needs Evict)
            way = getLRU(values.index);
            switch (cache[values.index].lines[way].MESI) {
                case 'M':
                    // Send message to L1
                    // Write modified value to memory
                    // Set tag & MESI
                    // send message to L1
                    messageToCache(EVICTLINE, address);
                    busOperation(WRITE, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'E':
                    // Send message to L1
                    // Read from memory
                    // Set tag & MESI, update LRU
                    // send message to L1
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'S':
                    // Send message to L1
                    // Read from memory
                    // Set tag & MESI, update LRU
                    // send message to L1
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    cache[values.index].lines[way].MESI = 'S';
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
                case 'I':
                    // Send message to L1
                    // Read from memory
                    // Set tag & MESI, update LRU
                    // send message to L1
                    messageToCache(EVICTLINE, address);
                    busOperation(READ, address, &snoopResult);
                    writeMESI(values.index, way, &snoopResult);
                    cache[values.index].lines[way].tag = values.tag;
                    updateLRU(values.index, way);
                    messageToCache(SENDLINE, address);
                    break;
            }
        }
    }
}

void write(unsigned int address) {
    Derived values;
    int way;
    int snoopResult;
    output.cacheWrites++;

    // Parse address & check for hit in set
    values = addressParser(address);
    way = findHit(values);

    if (way != -1) {
        // Hit, update LRU and send message to L1
        output.cacheHits++;
        switch (cache[values.index].lines[way].MESI) {
        case 'M':
            updateLRU(values.index, way);
            messageToCache(GETLINE, address);
            break;
        case 'E':
            updateLRU(values.index, way);
            cache[values.index].lines[way].MESI = 'M';
            messageToCache(GETLINE, address);
            break;
        case 'S':
            busOperation(INVALIDATE, address, &snoopResult);
            cache[values.index].lines[way].MESI = 'M';
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
            // Read from memory to line, set tag & MESI
            // send message to L1
            busOperation(RWIM, address, &snoopResult);
            messageToCache(GETLINE, address);
            cache[values.index].lines[way].MESI = 'M';
            cache[values.index].lines[way].tag = values.tag;
            updateLRU(values.index, way);
        }
        else {
            // Miss (Needs Evict)
            way = getLRU(values.index);
            switch (cache[values.index].lines[way].MESI) {
            case 'M':
                // Send message to L1
                // Write modified value to memory
                // RWIM from memory
                // Set tag & MESI
                // send message to L1
                messageToCache(EVICTLINE, address);
                busOperation(WRITE, address, &snoopResult);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            // Send message to L1
            // RWIM from memory
            // Set tag & MESI
            // send message to L1
            case 'E':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            case 'S':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            case 'I':
                messageToCache(EVICTLINE, address);
                busOperation(RWIM, address, &snoopResult);
                cache[values.index].lines[way].MESI = 'M';
                cache[values.index].lines[way].tag = values.tag;
                updateLRU(values.index, way);
                messageToCache(GETLINE, address);
                break;
            }

        }
    }
}

void snoopInvaild(unsigned int address) {
    Derived values = addressParser(address);
    int way = findHit(values);
    int snoopResult;

    if (way != -1) {
        // Hit
        if (cache[values.index].lines[way].MESI == 'M') {
            putSnoopResult(address, HITM);
            busOperation(WRITE, address, &snoopResult);
        }
        else {
            putSnoopResult(address, HIT);
        }
        cache[values.index].lines[way].MESI = 'I';
        messageToCache(INVALIDATELINE, address);
    }
    else {
        // Miss
        putSnoopResult(address, NOHIT);
    }
}

void snoopRead(unsigned int address) {
    Derived values = addressParser(address);
    int way = findHit(values);
    int snoopResult;

    if (way != -1) {
        // Hit
        if (cache[values.index].lines[way].MESI != 'M') {
            putSnoopResult(address, HIT);
            cache[values.index].lines[way].MESI = 'S';
        }
        else {
            putSnoopResult(address, HITM);
            cache[values.index].lines[way].MESI = 'S';
        }
        updateLRU(values.index, way);
    }
    else {
        // Miss
        putSnoopResult(address, NOHIT);
    }
}

void snoopWrite(unsigned int address) {
    Derived values = addressParser(address);
    int way = findHit(values);
    int snoopResult;

    if (way != -1) {
        // Hit
        if (cache[values.index].lines[way].MESI == 'M') {
            putSnoopResult(address, HITM);
            busOperation(WRITE, address, &snoopResult);
        }
        else {
            putSnoopResult(address, HIT);
        }
        cache[values.index].lines[way].MESI = 'I';
        updateLRU(values.index, way);

    }
    else {
        // Miss
        putSnoopResult(address, NOHIT);
    }
}

void snoopReadM(unsigned int address) {
    Derived values = addressParser(address);
    int way = findHit(values);
    int snoopResult;

    if (way != -1) {
        // Hit
        if (cache[values.index].lines[way].MESI == 'M') {
            putSnoopResult(address, HITM);
            busOperation(WRITE, address, &snoopResult);
        }
        else {
            putSnoopResult(address, HIT);
        }
        cache[values.index].lines[way].MESI = 'I';
        updateLRU(values.index, way);

    }
    else {
        // Miss
        putSnoopResult(address, NOHIT);
    }
}

void updateLRU(unsigned int setIndex, int way) {
    unsigned int PLRU = cache[setIndex].PLRU;
    // Zero correct bits, then set those bits to the correct value
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
    cache[setIndex].PLRU = PLRU;
}

int getLRU(unsigned int setIndex) {
    int way;
    unsigned int PLRU = cache[setIndex].PLRU;
    
    // Check bit and decide where to check next bit
    if( (PLRU & 0x1) == 0 ) {
        if( ((PLRU >> 2) & 0x1) == 0 ) {
            if( ((PLRU >> 3) & 0x1) == 0 ) {
                way = 6;
            }
            else {
                way = 7;
            }
        }
        else {
            if( ((PLRU >> 6) & 0x1) == 0 ) {
                way = 5;
            }
            else {
                way = 4;
            }
        }
    }
    else {
        if( ((PLRU >> 1) & 0x1) == 0 ) {
            if( ((PLRU >> 5) & 0x1) == 0 ) {
                way = 3;
            }
            else {
                way = 2;
            }
        }
        else {
            if( ((PLRU >> 4) & 0x1) == 0 ) {
                way = 0;
            }
            else {
                way = 1;
            }
        }
    }
    return way;
}

int findEmpty(unsigned int setIndex) {
    int way;

    for (int i = 0; i < NUMWAYS; i++) {
        // Check for first invalid line
        if (cache[setIndex].lines[i].MESI == 'I') {
            return way = i;
        }
    }
    return -1;
}

int findHit(Derived values) {
    int way;
    for (int i = 0; i < NUMWAYS; i++) {
        // Check if tag matches & line is not invalid
        if (cache[values.index].lines[i].tag == values.tag && cache[values.index].lines[i].MESI != 'I') {
            return way = i;
        }
    }
    return -1;
}

void printValid(unsigned int address) {
    Derived values;
    values = addressParser(address);
    printf("\n===================================================================\n");
    printf("|                          Cache Set: %d                        |", values.index);
    printf("\n===================================================================\n");
    for (int i = 0; i < NUMWAYS; i++) {
        if (cache[values.index].lines[i].MESI == 'I') {
            continue;
        }
        else {
            printf("                            ------------\n");
            printf("                            Line   : %d\n",    i);
            printf("                            Tag    : 0x%x\n",  cache[values.index].lines[i].tag);
            printf("                            MESI   : %c\n",  cache[values.index].lines[i].MESI);
            printf("                            ------------\n\n");
        }
    }
}

void printOutput() {
    printf("\n===================================================================\n");
    printf("|                              Stats                              |");
    printf("\n===================================================================\n");
    printf("                    Total Cache Reads:    %d\n",    output.cacheReads);
    printf("                    Total Cache Writes:   %d\n",      output.cacheWrites);
    printf("                    Total Cache Hits:     %d\n",      output.cacheHits);
    printf("                    Total Cache Misses:   %d\n",      output.cacheMiss);
    printf("                    Cache Hit Ratio:      %.2f\n\n",  output.cacheRatio);
}

void resetCache() {
    for (int i = 0; i < NUMSETS; i++) {
        for (int j = 0; j < NUMWAYS; j++) {
            cache[i].lines[j].MESI = 'I';
            cache[i].lines[j].tag = 0;
        }
    }

    output.cacheHits = 0;
    output.cacheMiss = 0;
    output.cacheRatio = 0;
    output.cacheReads = 0;
    output.cacheWrites = 0.0;
}

void writeMESI(unsigned int setIndex, int way, int *snoopResult) {
    if (*snoopResult == NOHIT) {
        cache[setIndex].lines[way].MESI = 'E';
    }
    else if (*snoopResult == HIT || *snoopResult == HITM) {
        cache[setIndex].lines[way].MESI = 'S';
    }
}

void messageToCache(int Message, unsigned int Address) {
    if(!silentMode) {
        printf("L2: %s %x\n", messageChar[Message], Address);
    }
}

void busOperation(int BusOp, unsigned int Address, int *SnoopResult) {
    *SnoopResult = getSnoopResult(Address);
    if (!silentMode) {
        printf("BusOp: %s, Address : %x, Snoop Result : %s\n", busChar[BusOp], Address, snoopChar[*SnoopResult]);
    }
}

int getSnoopResult(unsigned int Address) {
    int snoopRes;

    // Bitmask to parse address
    snoopRes = (Address & BYTEMASK);

    switch (snoopRes) {
        case NOHIT:
            return NOHIT;
        case HIT:
            return HIT;
        case HITM:
            return HITM;
        default:
            return NOHIT;
    }
}

void putSnoopResult(unsigned int Address, int SnoopResult) {
    if (!silentMode) {
        printf("SnoopResult: Address %x, SnoopResult : %s\n", Address, snoopChar[SnoopResult]);
    }
}

Derived addressParser(unsigned int address) {
    Derived values;
    
    // Bitmask to parse address
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

    // Reads and parses file into struct
    while(fscanf(fp, "%d %x", &trace.command[i], &trace.address[i]) != EOF) {
        i++;
    }
    trace.size = i;

    return trace;
}





