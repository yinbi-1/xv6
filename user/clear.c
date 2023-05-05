#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void clear() {
    fprintf(1, "\033[2J\033[1;1H");
}

int main(int argc, char *argv[]){
    clear();

    exit(0); 
}
