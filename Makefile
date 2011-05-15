all: neroripper

neroripper: main.o
  cc -o neroripper main.o

main.o: main.c
  cc -Wall -c -o main.o main.c
