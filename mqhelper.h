#ifndef MQ_HELPER
#define MQ_HELPER

#include <stdio.h>
#include <stdlib.h>

#define MAX_MSG_SIZE 255
#define MAX_MESSAGES 10
static inline void die(const char*str) {
    perror(str);
    exit(1);
}

#endif
