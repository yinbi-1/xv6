#include "kernel/types.h"
#include "user/user.h"

int 
main (int argc, char * argv[]) {
    if (argc < 2) {
        fprintf(1, "usage: echo_reversal <character string>\n");
        exit(0);
    }

    for (int i = 1; i < argc; ++i) {
        int n = strlen(argv[i]);
        for (int j = n - 1; j >= 0; --j)
            fprintf(1, "%c", argv[i][j]);
        i < argc - 1 ? fprintf(1, "%s", " ") : fprintf(1, "%s", "\n");
    }

    exit(0);
}