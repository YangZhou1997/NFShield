# !/bin/bash

for i in test_file
do
    arm-linux-gnueabi-gcc -o $i $i.c --static -O3
done