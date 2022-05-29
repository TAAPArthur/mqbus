#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
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
        if (ret == -1) {
            if(errno == EAGAIN) {
                continue;
            }
            return -1;
        }
        if(ret == 0)
            return size;
        size += ret;
    }
    return size;
}

void _send_helper(mqd_t mqd, int priority, const char* message, int size) {
    if(mq_send(mqd, message, size, priority) == -1)
        die("mq_send failed to send message");
}

void send(mqd_t mqd, int priority, const char* message) {
    if(!message) {
        int size = 0;
        int index = 0;
        static char buffer[2][MAX_MSG_SIZE];
        int read_more;
        do {
            read_more = 0;
            size += readAll(STDIN_FILENO, buffer[index] + size, MAX_MSG_SIZE - size);
            if (size == -1) {
                die("failed to read stdin");
            } else if (size == 0) {
                break;
            } else if (size == MAX_MSG_SIZE) {
                // there might be more data to read
                read_more = 1;
                char* ptr = strrchr(buffer[index], '\n');
                if(ptr) {
                    size = ptr - buffer[index] + 1;
                }
            }
            _send_helper(mqd, priority, buffer[index], size);
            if (read_more) {
                // Copy remaining data in buffer to the alt buffer and account for its size
                if(MAX_MSG_SIZE != size) {
                    memcpy(buffer[!index], buffer[index] + size, MAX_MSG_SIZE - size);
                }
                size = MAX_MSG_SIZE - size;
                index = !index;
            }
        } while(read_more);
    } else {
        _send_helper(mqd, priority, message, MIN(MAX_MSG_SIZE, strlen(message)));
    }
}

void usage(void) {
    printf("mq: (-s|-r) [-p PRIORITY] [ -m MASK ] name [MESSAGE]\n");
    printf("mqsend: [-p PRIORITY] name [ -m MASK ] [MESSAGE]\n");
    printf("mqreceive: [-p PRIORITY] [ -m MASK ] name\n");
}

int main(int argc, const char* argv[]) {
    int receiveFlag;
    int priority = 0;
    char name[255] = {0};
    int mask = DEFAULT_CREATION_MASK;
    const char* message = parseArgs(argv, &receiveFlag, &priority, &mask, name);
    mqd_t mqd = mq_open(name, (!receiveFlag?O_WRONLY:O_RDONLY)|O_CLOEXEC|O_CREAT, mask, &attr);
    if(mqd == -1)
        die("mq_open failed to open message queue");
    if(!receiveFlag)
        send(mqd, priority, message);
    else
        receive(mqd);
}
