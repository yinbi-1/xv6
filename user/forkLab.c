#include "kernel/types.h"
#include "user/user.h"


int main() {
    if (fork() == 0) {
        sleep(300);
        exit(0);
    }else {
        if (fork() == 0) {
            sleep(300);
            exit(0);
        }
    }
    sleep(300);
    wait(0);
    wait(0);
    
    exit(0);
}