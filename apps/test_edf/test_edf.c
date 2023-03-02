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

#define gettid() syscall(__NR_gettid)
#define SET_AFFINITY_SYSCALL 241
#define SET_EDF_SYSCALL 437
#define SCHED_DEADLINE 6

struct sched_attr {
     __u32 size;
     __u32 sched_policy;
     __u64 sched_flags;

     __s32 sched_nice;

     __u32 sched_priority;

     __u64 sched_runtime;
     __u64 sched_deadline;
     __u64 sched_period;
};

int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags) {
     return syscall(__NR_sched_setattr, pid, attr, flags);
}

int sched_setaffinity(pid_t pid, size_t cpusetsize, const cpu_set_t *mask) {
     return syscall(SET_AFFINITY_SYSCALL, pid, cpusetsize, mask);
}

int set_edf(pid_t pid, unsigned int C, unsigned int T, int cpu_id) {
	return syscall(SET_EDF_SYSCALL, pid, C, T, cpu_id);
}

int main (int argc, char **argv) {
	assert(argc == 5);
	pid_t pid = atoi(argv[1]);
	unsigned int execution_time = atoi(argv[2]);
	unsigned int period = atoi(argv[3]);
	int cpu_id = atoi(argv[4]);


	assert(period >= execution_time);
	assert(period >= 0);
	assert(period <= 10000);
	assert(execution_time >= 0);
	assert(execution_time <= 10000);

	if (set_edf(pid, execution_time, period, cpu_id) < 0) {
		perror("SET_EDF_SYSCALL");
		exit(-1);
	}


	printf("PID: %u, C: %u, T: %u\n", pid, execution_time, period); 
	

	return 0;
}