CC=gcc
CROSS_COMPILE=arm-linux-gnueabihf-

all:
	$(CROSS_COMPILE)$(CC) -Wall -pthread multicore_periodic.c -o multicore_periodic

clean:
	rm multicore_periodic