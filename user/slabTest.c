#include "kernel/types.h"
#include "user/user.h"

int main() {
    char *a1 = myalloc(16);
    printf("16 bytes %p\n", a1);
    myfree(a1);
    exit(0);
}