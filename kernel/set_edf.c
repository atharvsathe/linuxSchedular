#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/cpumask.h>
#include <linux/sched.h>

#define SCHED_DEADLINE 	6
#define SCHED_FIFO		1
#define SCHED_RR		2
#define MAX_SIZE 		12


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

typedef struct sched_attr_store  {
	pid_t pid;
	uint32_t policy;
	uint32_t C;
    uint32_t T;
	
} sched_attr_store;

static sched_attr_store all_attr[MAX_SIZE];
static uint32_t position = 0;

SYSCALL_DEFINE4(set_edf, pid_t __user, pid, unsigned int __user, C, unsigned int __user, T, int __user, cpuid) {
	struct task_struct *p, *process, *t;
	struct cpumask setcpu;
	struct sched_attr attr;
	int all_C = 1;
	int all_T = 1;
	int base = 1000000;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();

	all_attr[position].pid = pid;
	all_attr[position].policy = p->policy;
	all_attr[position].C = C;
	all_attr[position].T = T;
	position = (position + 1) % MAX_SIZE;

	cpumask_clear(&setcpu);
	cpumask_set_cpu((unsigned)cpuid, &setcpu);
	if(sched_setaffinity(pid, &setcpu) < 0) {
	  return -1;
	}

	for_each_process_thread(process, t) {
		if (t->policy == SCHED_DEADLINE) {
			int runtime = t->dl.dl_runtime;
			int period = t->dl.dl_period;
			runtime = runtime / base;
			period = period / base;
			if (cpumask_subset(&setcpu, &(t->cpus_mask)) == 1 && runtime > 0) {
				all_C = (all_T * runtime) + (all_C * period);
				all_T = all_T * period;
				if (all_C > 1000 && all_T > 1000) {
					all_C = all_C / 100;
					all_T = all_T / 100;
				}
			}
		}
	}

	all_C = (all_T * C) + (all_C * T);
	all_T = all_T * T;
	if (all_C / 2 > all_T) {
		printk("Set_edf: Not schedulable");
		return -1;
	}

	attr.size = sizeof(struct sched_attr);
	attr.sched_flags = 0;
	attr.sched_nice = 0;
	attr.sched_priority = 0;

	attr.sched_policy = SCHED_DEADLINE;
	attr.sched_runtime = C * 1000 * 1000;
	attr.sched_period = attr.sched_deadline = T * 1000 * 1000;

	if (sched_setattr(p, &attr) < 0) {
		return -1;
	}
	return 0;
}

SYSCALL_DEFINE1(cancel_edf, pid_t __user, pid) {
	uint32_t count = 0;
	struct task_struct *p;
	struct sched_attr attr;

	rcu_read_lock();
	p = find_task_by_vpid(pid);
	rcu_read_unlock();

	for (count = 0; count < MAX_SIZE; count++) {
		if (all_attr[count].pid == pid) {
			attr.size = sizeof(struct sched_attr);
			attr.sched_flags = 0;
			attr.sched_nice = 0;
			attr.sched_priority = 0;

			attr.sched_policy = all_attr[count].policy;
			attr.sched_runtime = 0;
			attr.sched_period = attr.sched_deadline = all_attr[count].T * 1000 * 1000;

			if(sched_setattr(p, &attr) < 0) {
				return -1;
			}
			all_attr[count].pid = -1;
			printk("Cancel EDF:%d", (int)all_attr[count].pid);
			break;
		}
	}

	return 0;
}