# linuxSchedular
Energy Aware Schedular with Dynamic Frequency Scaling
- Modifications to kernel include accomodating EDF for multicore while maintaing schedulability.
- Kernel module iterates over all tasks to print them for the user
- User level application is able to schedule a given task set for n cores using EDF and Dynamic Frequency Scaling to reduce power consumption
- The report contains the timing diagrams for various task sets and comparisons extracted using trace-cmd tool

