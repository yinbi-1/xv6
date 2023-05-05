#include "kernel/types.h"
#include "user/user.h"

int main() {
    if (fork() == 0) {
        sleep(10);
        exit(0);
    }else exit(0);
}