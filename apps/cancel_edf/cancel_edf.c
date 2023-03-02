#define _GNU_SOURCE
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <sys/syscall.h>

#include <assert.h>
#include <sched.h>

#define CANCEL_EDF_SYSCALL 438


int cancel_edf(pid_t pid) {
	return syscall(CANCEL_EDF_SYSCALL, pid);
}

int main (int argc, char **argv) {
	assert(argc == 2);
	pid_t pid = atoi(argv[1]);
	
	if (cancel_edf(pid) < 0){
		perror("CANCEL_EDF_SYSCALL");
		exit(-1);
	}

	return 0;
}