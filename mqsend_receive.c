#include <fcntl.h>
#include <poll.h>
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

    struct pollfd fds[2] =  {{.fd = mqd, .events = POLLIN}, {.fd = STDOUT_FILENO} };
    while(1){
        int ret = poll(fds, 2, -1);
        if(fds[0].revents & POLLIN) {
            ret = mq_receive(mqd,(char*) &buffer, MAX_MSG_SIZE, NULL);
            if(ret == -1) {
                die("mq_receive failed to retrieve message");
            }
            if(write(STDOUT_FILENO, buffer, ret)==-1) {
                die("failed to echo message out to stdout");
            }
        }
        else
            exit(2); // die on STDOUT hangup
    }
}
int readAll(int fd, char* buffer, size_t bufferSize) {
    int size = 0;
    while(size < bufferSize) {
        int ret = read(fd, buffer + size, bufferSize - size);
        if (ret == -1)
            return -1;
        if(ret == 0)
            return size;
        size += ret;
    }
    return size;
}

void send(mqd_t mqd, int priority, const char* message) {
    char buffer[MAX_MSG_SIZE];
    int size;
    if(!message) {
        size = readAll(STDIN_FILENO, buffer, sizeof(buffer));
        if (size == -1)
            die("failed to read stdin");
        message = buffer;
    } else {
        size = MIN(MAX_MSG_SIZE, strlen(message));
    }
    if(mq_send(mqd, message, size, priority) == -1)
        die("mq_send failed to send message");
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
        die("mq_open failed to open message queue");
    if(!receiveFlag)
        send(mqd, priority, message);
    else
        receive(mqd);
}
