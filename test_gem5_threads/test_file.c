#include <stdio.h>
#include <stdlib.h>

void main(){
    FILE *file = fopen("/users/yangzhou/NF-GEM5/test.dat", "w");
    fprintf(file, "hello gem5\n");
    return;
}