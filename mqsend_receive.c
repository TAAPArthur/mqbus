#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mqhelper.h"

#define MAX_MSG_SIZE 255
const struct mq_attr attr = {
    .mq_maxmsg = MAX_MESSAGES,
    .mq_msgsize = MAX_MSG_SIZE
};

void receive(mqd_t mqd) {
    char buffer[MAX_MSG_SIZE];
    while(1){
        int ret = mq_receive(mqd,(char*) &buffer, MAX_MSG_SIZE, NULL);
        if(ret == -1) {
            die("mq_receive");
        }
        if(write(STDOUT_FILENO, buffer, ret)==-1) {
            die("write");
        }
    }
}

void send(mqd_t mqd, int priority, const char* message) {
    char buffer[MAX_MSG_SIZE];
    if(!message) {
        int ret = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (ret == -1)
            die("failed read");
        buffer[sizeof(buffer)-1] = 0;
        message = buffer;
    }
    if(mq_send(mqd, message, MIN(MAX_MSG_SIZE, strlen(message)), priority) == -1)
        die("mq_send");
}

void usage(void) {
    printf("mq: (-s|-r) [-p PRIORITY] name [MESSAGE]\n");
    printf("mqsend: [-p PRIORITY] name [MESSAGE]\n");
    printf("mqreceive: [-p PRIORITY] name\n");
}

int main(int argc, const char* argv[]) {
    int receiveFlag;
    int priority = 0;
    char name[255] = {0};
    const char* message = parseArgs(argv, &receiveFlag, &priority, name);
    mqd_t mqd = mq_open(name, (!receiveFlag?O_WRONLY:O_RDONLY)|O_CLOEXEC|O_CREAT, 0722, &attr);
    if(mqd == -1)
        die("mqd");
    if(!receiveFlag)
        send(mqd, priority, message);
    else
        receive(mqd);
}
