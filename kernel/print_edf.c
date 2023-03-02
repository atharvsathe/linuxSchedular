#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/cpumask.h>
#include <linux/sched/signal.h>

#define SCHED_DEADLINE 6
#define NUM_CORES 4

SYSCALL_DEFINE1(print_edf, pid_t __user, pid) {
	struct task_struct *p, *t;

	for_each_process_thread(p, t) {
		if (t->policy == SCHED_DEADLINE && (pid == t->pid || pid == -1)) {
			int runtime = t->dl.dl_runtime;
			int period = t->dl.dl_period;
			int base = 1000000;

			int C = runtime / base;
			int T = period / base;

			struct cpumask mask;
			uint32_t cpuid = 0;
			uint32_t count = 0;

			for (count = 0; count < NUM_CORES; count++) {
				cpumask_clear(&mask);
				cpumask_set_cpu(count, &mask);
				if (cpumask_subset(&mask, &(t->cpus_mask)) == 1) {
					cpuid += count;
					break;
				}
			}
			if (C > 0) {
				printk("print_edf: PID: %d C: %d ms, T %d ms CPUID: %u", (int)t->pid, C, T, cpuid);
			}
		}
	}
	return 0;
}