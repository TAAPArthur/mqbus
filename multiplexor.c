#include <mqueue.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mqhelper.h"

#define MAX_LISTENERS 1024
#define READ_SIZE 1024

struct mq_attr attr = {
    .mq_maxmsg = 10,
    .mq_msgsize = READ_SIZE
};
static char buffer[READ_SIZE];
int listeners[MAX_LISTENERS];
int numListeners = 0;
void removeListener(int i) {
    for(;i<numListeners; i++) {
        listeners[i]=listeners[i+1];
    }
    numListeners--;
}
int baseFD;
void forwardInput(int fd) {
    int ret = read(fd, buffer, sizeof(buffer));
    for(int i=numListeners-1;i>=0 ;i--){
        if(write(listeners[i], buffer, ret) == -1) {
            perror("write");
            removeListener(i);
        }
    }
}
void addListener(int mqd) {
    int ret = mq_receive(mqd,(char*) &buffer, sizeof(buffer), NULL);
    if(ret==-1) {
        die("Failed to receive msg");
    }
    int fd = openat(baseFD, buffer, O_WRONLY|O_NONBLOCK);
    if(fd == -1) {
        perror("Failed to open pipe");
        return;
    }
    unlinkat(baseFD, buffer, 0);
    listeners[numListeners++] = fd;
}
void multiSend(int mqd) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    sigaction(SIGPIPE, &sa, NULL);
    struct pollfd fds[2] = {
        {.fd=STDIN_FILENO, .events=POLLIN},
        {.fd=mqd, .events=POLLIN}
    };
    void(*func[])(int) = {
        forwardInput,
        addListener
    };
    while(1) {
        if(poll(fds, 2, -1) == -1) {
            die("poll");
        }
        for(int i=0;i<2;i++) {
            if(fds[i].revents & POLLIN)
                func[i](fds[i].fd);
        }
    }
}

void registerForMultiRead(int mqd) {
    if(mkfifoat(baseFD, buffer, 0600)==-1) {
        die("mkfifoat");
    }

    int fd = openat(baseFD, buffer, O_RDONLY|O_NONBLOCK);
    if(fd == -1){
        die("failed to open pipe");
    }
    if(mq_send(mqd, buffer, strlen(buffer) + 1, 1) == -1){
        die("mq_send");
    }

    struct pollfd fds[1] =  {{.fd=fd, .events=POLLIN}};
    while(1) {
        int ret = poll(fds, 1, -1);
        if(fds[0].revents & POLLIN) {
            ret = read(fd, buffer, sizeof(buffer));
            if(ret == -1) {
                perror("read");
                registerForMultiRead(mqd);
                return;
            }
            if(write(STDOUT_FILENO, buffer, ret) == -1){
                die("write");
            }
        } else {
            close(fd);
            registerForMultiRead(mqd);
            return;
        }
    }
}
int main(int argc, const char* argv[]) {
    if(argc ==  1) {
        exit(1);
    }

    int baseFlag = 0;
    if(strstr(argv[0], "mqmultisend"))
        baseFlag = O_RDONLY;
    else if(strstr(argv[0], "mqmultirec"))
        baseFlag = O_WRONLY;

    const char baseDir[]="/tmp/multiplexor";
    mkdir(baseDir, 0777);
    int dir = open(baseDir, O_RDONLY);
    if(dir==-1){
        die("open (dir)");
    }
    if(mkdirat(dir, argv[1], 0744)==-1){
        if(errno != EEXIST) {
            die("mkdirat");
        }
    }

    baseFD = openat(dir, argv[1], O_RDONLY, 0744);
    if(baseFD  == -1) {
        die("openat");
    }

    char name[255] = {"/"};
    strcat(name, argv[1]);
    mqd_t mqd = mq_open(name, baseFlag|O_CREAT, 0722,  &attr);
    if(mqd == -1) {
        die("mq_open");
    }

    switch(baseFlag ) {
        case O_WRONLY:
            registerForMultiRead(mqd);
            break;
        case O_RDONLY:
            multiSend(mqd);
            break;
        default:
            exit(2);
    }
}
