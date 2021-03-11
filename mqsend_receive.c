#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mqhelper.h"
struct mq_attr attr = {
    .mq_maxmsg = MAX_MESSAGES,
    .mq_msgsize = MAX_MSG_SIZE
};

char buffer[MAX_MSG_SIZE];
void receive(mqd_t mqd) {
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

void send(mqd_t mqd) {
    int ret = read(STDIN_FILENO, buffer, sizeof(buffer));
    if (ret == -1)
        die("failed read");
    buffer[sizeof(buffer)-1] = 0;
    if(mq_send(mqd, buffer, strlen(buffer) + 1, 1) == -1)
        die("mq_send");
}

int main(int argc, char* argv[]) {
    if(argc == 1)
        exit(1);
    int baseFlag = -1;
    if(strstr(argv[0], "mqsend"))
        baseFlag = O_WRONLY;
    else if(strstr(argv[0], "mqreceive"))
        baseFlag = O_RDONLY;
    char name[255] = {"/"};
    strcat(name, argv[1]);
    mqd_t mqd = mq_open(name, baseFlag|O_CLOEXEC|O_CREAT, 0722, &attr);
    switch(baseFlag) {
        case O_WRONLY:
            send(mqd);
            break;
        case O_RDONLY:
            receive(mqd);
            break;
        default:
            exit(2);
    }
}
