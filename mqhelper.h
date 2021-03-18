#ifndef MQ_HELPER
#define MQ_HELPER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MESSAGES 10

#define MIN(A,B) (A<B?A:B)
static inline void die(const char*str) {
    perror(str);
    exit(1);
}

void usage(void);

const char* parseArgs(const char* argv[], int *receiveFlag, int*priority, char* name) {
    if(receiveFlag)
        *receiveFlag = strstr(argv[0], "receive")? 1:0;
    for(argv++ ; argv[0] && argv[0][0] == '-' && argv[0][1] != '-'; argv++)
        switch(argv[0][1]) {
            case 'h':
                usage();
                exit(0);
                break;
            case 'p':
                if(priority)
                    *priority = atoi(argv[0][2]?argv[0]+2: *(++argv));
                break;
            case 'r':
                if(receiveFlag)
                    *receiveFlag = 1;
                break;
            case 's':
                if(receiveFlag)
                    *receiveFlag = 0;
                break;
            default:
                usage();
                exit(1);
        }

    if(!argv[0]){
        usage();
        exit(1);
    }
    if(argv[0][0] != '/')
        name[0] = '/';
    strncat(name, argv[0], 254);
    return argv[1];
}

#endif
