#!/bin/bash

mkdir programs 2> /dev/null

#Compile libray code
cd Control
for i in *.c; do
    gcc -c "$i"
done
cd -

#Now compile all the programs
for i in *.c; do
    gcc -c "$i"
    gcc *.o Control/*.o -o programs/${i%%.*}
    rm *.o
done

#remove all evidence
rm Control/*.o