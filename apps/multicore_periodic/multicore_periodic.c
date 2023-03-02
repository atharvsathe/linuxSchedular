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
#include <pthread.h>
#include <math.h>

#define NUM_CORES 4
#define NUM_TASKS 10
#define SET_EDF_SYSCALL 437
#define gettid() syscall(__NR_gettid)

struct task_data {
     unsigned int C;
     unsigned int T;
     int cpuid;
};

struct cpu_util {
     int cpuid;
     float utilization;
};

struct cpu_util all_cores[NUM_CORES] = {{0, 0.0}, {1, 0.0}, {2, 0.0}, {3, 0.0}};

int set_edf(pid_t pid, unsigned int C, unsigned int T, int cpu_id) {
     return syscall(SET_EDF_SYSCALL, pid, C, T, cpu_id);
}

int comparison_bfd_func (const void * a, const void * b) {
     float fa = ((struct cpu_util *)a)->utilization;
     float fb = ((struct cpu_util *)b)->utilization;
     if (fa > fb) return -1;
     else if (fb < fa)return 1;
     else return 0;
}

int comparison_bfi_func (const void * a, const void * b) {
     float fa = ((struct cpu_util *)a)->utilization;
     float fb = ((struct cpu_util *)b)->utilization;
     if (fa > fb) return 1;
     else if (fb < fa)return -1;
     else return 0;
}

int compute_bfd_cpu(unsigned int C, unsigned int T){
     float task_utilisation = (float)C / T;
     qsort((struct cpu_util *)all_cores, NUM_CORES, sizeof(struct cpu_util), comparison_bfd_func);

     for(int i = 0; i < NUM_CORES; i++) {
          if (all_cores[i].utilization + task_utilisation < 1) {
               all_cores[i].utilization += task_utilisation;
               return all_cores[i].cpuid;
          }
     }
     return -1;
}

int compute_bfi_cpu(unsigned int C, unsigned int T) {
     float task_utilisation = (float)C / T;
     qsort((struct cpu_util *)all_cores, NUM_CORES, sizeof(struct cpu_util), comparison_bfi_func);

     for(int i = 0; i < NUM_CORES; i++) {
          if (all_cores[i].utilization + task_utilisation < 1) {
               all_cores[i].utilization += task_utilisation;
               return all_cores[i].cpuid;
          }
     }
     return -1;
}

int max(int a, int b) {
     if (a > b) return a;
     return b;
}

void change_freq() {
     qsort((struct cpu_util *)all_cores, NUM_CORES, sizeof(struct cpu_util), comparison_bfd_func);
     float ratio = 1 / all_cores[0].utilization;
     int scaling_factor = max((int)(15 / ratio) + 1, 6);
     scaling_factor *= 100000;

     char freq[8];
     sprintf(freq, "%d", scaling_factor);

     char line[255];
     FILE *governer_file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "r");
     fgets(line, sizeof(line), governer_file);
     char *token = strtok(line, ",");

     if (strncmp(token, "userspace", 9) == 0) {
          printf("Userspace governor detected. Freq set to %s\n",freq);
          FILE *freq_file = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed", "w");
    
          fputs(freq, freq_file);
          fclose(freq_file);

     }
}    

void *run_deadline(void *data) {
     struct task_data *task = (struct task_data *) data;
     printf("PID: %ld C:%u T:%u CPUID:%d\n",  gettid(), task->C, task->T, task->cpuid);
    
     if (set_edf(gettid(), task->C, task->T, task->cpuid) < 0) {
          perror("SET_EDF_SYSCALL");
          exit(-1);
     }
     while(1) {}
     return NULL;
}

int main (int argc, char **argv) {
     char *filename = argv[1];
     unsigned int C[NUM_TASKS]; 
     unsigned int T[NUM_TASKS];
     pthread_t thread[NUM_TASKS];
     struct task_data temp[NUM_TASKS];

     FILE *fp = fopen(filename, "r");
     if (fp == NULL) {
          perror("File not opened");
          exit(-1);
     }

     char line[255];
     int count = 0;
     int CT = 0;
     fgets(line, sizeof(line), fp);

     while(fgets(line, sizeof(line), fp)) {
          char *token = strtok(line, ",");
          while(token != NULL) {
               if (CT % 2 == 0) 
                    C[count] = atoi(token);
               else
                    T[count] = atoi(token);
               token = strtok(NULL, ",");
               CT ++;
          }

          int cpuid = compute_bfi_cpu(C[count], T[count]);
          temp[count].C = C[count]; 
          temp[count].T = T[count];
          temp[count].cpuid = cpuid;
          
          pthread_create(&thread[count], NULL, run_deadline, (void *)&temp[count]);

          count++;
     }

     change_freq();

     while (1) {}
     for (int i = 0; i < NUM_TASKS; i++) {
         pthread_join(thread[i], NULL);
     }
     return 0;
}