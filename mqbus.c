#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mqhelper.h"

#define MAX_LISTENERS 1024
#define READ_SIZE 1024
#define MAX_PIPE_NAME_LEN 32

const struct mq_attr attr = {
    .mq_maxmsg = MAX_MESSAGES,
    .mq_msgsize = MAX_PIPE_NAME_LEN
};
int listeners[MAX_LISTENERS];
char pathNames[MAX_LISTENERS][MAX_PIPE_NAME_LEN];
int numListeners = 0;

void removeListener(int i, int baseFD, int unlink) {
    if(unlink)
        unlinkat(baseFD, pathNames[i], 0);
    close(listeners[i]);
    for(; i < numListeners; i++) {
        listeners[i] = listeners[i + 1];
        strcpy(pathNames[i + 1], pathNames[i]);
    }
    numListeners--;
}

void forwardMessage(int baseFD, const char* message, int size) {
    for(int i = numListeners - 1; i >= 0 ; i--) {
        if(write(listeners[i], message, size) == -1) {
            removeListener(i, baseFD, 1);
        }
    }
}

void forwardInput(int fd, int baseFD) {
    char buffer[READ_SIZE];
    int ret = read(fd, buffer, sizeof(buffer));
    if(ret == -1)
        die("Failed to read data to forward");
    forwardMessage(baseFD, buffer, ret);
}

void addListenerByName(int baseFD, char* pipeName) {
    int fd = openat(baseFD, pipeName, O_WRONLY|O_NONBLOCK);
    if(fd == -1) {
        unlinkat(baseFD, pipeName, 0);
        return;
    }
    for(int i = numListeners - 1; i >= 0; i--)
        if(strcmp(pipeName, pathNames[i]) == 0)
            removeListener(i, baseFD, 0);
    strncpy(pathNames[numListeners], pipeName, MAX_PIPE_NAME_LEN);
    listeners[numListeners++] = fd;
}
void addListener(int mqd, int baseFD) {
    if(numListeners == MAX_LISTENERS) {
        dprintf(STDERR_FILENO, "Too many listeners\n");
        return;
    }

    char pipeName[MAX_PIPE_NAME_LEN];
    int ret = mq_receive(mqd, pipeName, sizeof(pipeName), NULL);
    if(ret == -1) {
        die("Failed to receive msg");
    }
    pipeName[MAX_PIPE_NAME_LEN - 1] = 0;
    addListenerByName(baseFD, pipeName);
}

void loadExistingReaders(int baseFD) {
    DIR* dir = fdopendir(dup(baseFD));
    if(!dir)
        die("fdopendir");
    struct dirent *d;
    while ((d = readdir(dir))) {
        if (strcmp(d->d_name, ".") == 0 ||
            strcmp(d->d_name, "..") == 0)
            continue;
        addListenerByName(baseFD, d->d_name);
    }
    closedir(dir);
}
void multiSend(int mqd, int baseFD) {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    sigaction(SIGPIPE, &sa, NULL);

    struct pollfd fds[] = {
        {.fd = STDIN_FILENO, .events = POLLIN},
        {.fd = mqd, .events = POLLIN}
    };

    int numFDs = sizeof(fds)/sizeof(fds[0]);

    void(*func[])(int, int) = {
        forwardInput,
        addListener
    };
    while(1) {
        if(poll(fds, numFDs, -1) == -1) {
            die("poll");
        }
        for(int i = numFDs - 1; i >= 0; i--){
            if(fds[i].revents & POLLIN) {
                func[i](fds[i].fd, baseFD);
                break;
            }
            else if(fds[i].revents & (POLLERR|POLLHUP)) {
                exit(0);
            }
        }
    }
}

void registerForMultiRead(int mqd, int baseFD, int mask) {
    signal(SIGPIPE, SIG_IGN);
    char pidName[MAX_PIPE_NAME_LEN];
    sprintf(pidName, "%d", getpid());
    if(mkfifoat(baseFD, pidName, mask)==-1) {
        if(errno != EEXIST) {
            die("mkfifoat");
        }
    }

    int fd = openat(baseFD, pidName, O_RDWR|O_NONBLOCK);
    if(fd == -1){
        die("failed to open named fifo");
    }
    if(mq_send(mqd, pidName, strlen(pidName) + 1, 1) == -1){
        die("mq_send failed to send registration message");
    }

    struct pollfd fds[2] =  {{.fd = fd, .events = POLLIN}, {.fd = STDOUT_FILENO} };

    char buffer[READ_SIZE];
    while(1) {
        int ret = poll(fds, 2, -1);
        if(ret == -1)
            die("poll");
        if( ret != -1 && (fds[0].revents & POLLIN)) {
            if((ret = read(fd, buffer, READ_SIZE)) != -1 && write(STDOUT_FILENO, buffer, ret) != -1) {
                continue;
            }
        }
        close(fd);
        unlinkat(baseFD, pidName, 0);
        exit(2);
    }
}

void usage(void) {
    printf("mqbus: (-s|-r) name [ -m MASK ] [MESSAGE]\n");
    printf("mqbus-send: name [ -m MASK ] [MESSAGE]\n");
    printf("mqbus-receive: name\n");
}

static int createAndOpenDir(const char* relativePath, int mask) {
    char* baseDir=getenv("MQBUS_DIR");
    if(!baseDir)
        baseDir="/tmp/mqbus";
    mkdir(baseDir, 0777);
    int dir = open(baseDir, O_RDONLY);
    if(dir == -1) {
        die("open (dir)");
    }
    if(mkdirat(dir, relativePath, mask) == -1) {
        if(errno != EEXIST) {
            die("mkdirat");
        }
    }

    int baseFD = openat(dir, relativePath, O_RDONLY);
    if(baseFD == -1) {
        die("openat");
    }
    close(dir);
    return baseFD;
}

int main(int argc, const char* argv[]) {
    int receiveFlag;
    char name[255] = {0};
    int mask = DEFAULT_CREATION_MASK;
    const char* message = parseArgs(argv, &receiveFlag, NULL, &mask, name);
    int baseFD = createAndOpenDir(name + 1, mask | 0111);
    mqd_t mqd = mq_open(name, (receiveFlag ? O_WRONLY : O_RDONLY) | O_CREAT, mask, &attr);
    if(mqd == -1) {
        die("mq_open failed to open message queue");
    }
    if(receiveFlag)
        registerForMultiRead(mqd, baseFD, mask);
    else {
        loadExistingReaders(baseFD);
        if(message)
            forwardMessage(mqd, message, strlen(message));
        else
            multiSend(mqd, baseFD);
    }
}
