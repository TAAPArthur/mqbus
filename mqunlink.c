
#include <mqueue.h>
#include "mqhelper.h"

void usage(void) {
    printf("mqunlink: name\n");
}

int main(int argc, const char* argv[]) {
    char name[255] = {0};
    parseArgs(argv, NULL, NULL, name);
    int ret = mq_unlink(name);
    if(ret == -1) {
        die("Failed to unlink");
    }
}
