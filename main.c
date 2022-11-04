#include "main.h"

int main(int argc, char *argv[]) {
    char *filename;
    traceValues trace;

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
                    silentModeGbl = true;
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

    return 0;
}

traceValues traceParser(char *filename) {
    FILE *fp;
    traceValues trace;
    int i = 0;
    fp = fopen(filename, "r");

    if(fp == NULL) {
        printf("Unable to open file!\n");
        exit(-1);
    }

    if(silentModeGbl == false) {
        printf("===Command===|===Address===\n");
    }

    // Reads and parses file into struct
    while(fscanf(fp, "%d %x", &trace.command[i], &trace.address[i]) != EOF) {
        if(silentModeGbl == false) {
            printf("      %d          %x\n", trace.command[i], trace.address[i]);
        }
        i++;
    }

    return trace;
}





