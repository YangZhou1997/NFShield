#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void core0_main() {
    int j = 1;
    for (int i = 0; i < 10000; i++) {
        j += 2;
    }
    printf("Hello from core 0 main at %d\n", j);
}

void core1_main() {
    for (int i = 0; i < 1; i++)
        printf("Hello from core 1 main at %d\n", i);
}

void core2_main() {
    int j = 1;
    for (int i = 0; i < 50000; i++) {
        j += 2;
    }
    printf("Hello from core 2 main at %d\n", j);
}

void core3_main() {
    for (int i = 0; i < 1; i++)
        printf("Hello from core 3 main at %d\n", i);
}

void thread_entry(int cid, int nc) {
    if (nc != 4) {
        if (cid == 0) {
            printf("This program requires 4 cores but was given %d\n", nc);
        }
        return;
    }
    printf("enter core %d/%d\n", cid, nc);

    if (cid == 0) {
        core0_main();
    } else if (cid == 1) {
        core1_main();
    } else if (cid == 2) {
        core2_main();
    } else {
        core3_main();
    }
}