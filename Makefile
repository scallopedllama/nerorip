all: neroripper

neroripper: main.o nrg.o util.o
	cc -Wall -Wextra -o neroripper main.o nrg.o util.o

main.o: main.c
	cc -Wall -Wextra -c -o main.o main.c

nrg.o: nrg.c
	cc -Wall -Wextra -c -o nrg.o nrg.c

util.o: util.c
	cc -Wall -Wextra -c -o util.o util.c
