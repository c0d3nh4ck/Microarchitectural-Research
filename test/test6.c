#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

uint64_t rdtsc_begin() {
    uint64_t a, d;
    asm volatile ("lfence");
    asm volatile ("CPUID");
    asm volatile ("rdtsc" : "=a" (a), "=d" (d));
    a = (d<<32) | a;
    asm volatile ("lfence");
    return a;
}

uint64_t rdtsc_end() {
    uint64_t a, d;
    asm volatile ("lfence");
    asm volatile ("rdtsc" : "=a" (a), "=d" (d));
    a = (d<<32) | a;
    asm volatile ("CPUID");
    asm volatile ("lfence");
    return a;
}

void flush(void* p) {
	asm volatile ("clflush 0(%0)" :: "c" (p) : "rax");
}


static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

int
main(int argc, char **argv)
{
    struct perf_event_attr pe;
    long long count;
    int fd;

    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = 0x0879;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
//    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }


    uint64_t t1, t2;
    uint64_t T_bh[10], T_ah[10];


    /* 1st access */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < 10; i++) {

        t1 = rdtsc_begin();

        __asm__ volatile(
            ".intel_syntax noprefix;"
            "xchg rax, rdx;"
            "nop dword ptr [rbx+rbx*1+0x0];"
            "nop word ptr [rbx+rbx*1+0x0];"
            "cmpxchg [rsp], rbx;");

        t2 = rdtsc_end();

        T_bh[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB before CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE before CH -> %lld\n", count);
   



    /* Consecutive accesses */

    for (int i=0; i < 10000; i++) {
        __asm__ volatile(
            ".intel_syntax noprefix;"
            "xchg rax, rdx;"
            "nop dword ptr [rbx+rbx*1+0x0];"
            "nop word ptr [rbx+rbx*1+0x0];"
            "cmpxchg [rsp], rbx;");
    }




    /* Last access */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < 10; i++) {

        t1 = rdtsc_begin();

        __asm__ volatile(
            ".intel_syntax noprefix;"
            "xchg rax, rdx;"
            "nop dword ptr [rbx+rbx*1+0x0];"
            "nop word ptr [rbx+rbx*1+0x0];"
            "cmpxchg [rsp], rbx;");

        t2 = rdtsc_end();

        T_ah[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB before CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE before CH -> %lld\n", count);
    

    /* Saving Data in a File */
    
    int avg1 = 0; 
    int avg2 = 0;

    FILE *fp = fopen("time.txt", "w");
    
    fprintf(fp, "---------------------------\n");
    fprintf(fp, "|  Time before cache hit  |\n");
    fprintf(fp, "---------------------------\n");
    
    for (int i=0; i<10; i++)
    {
        fprintf(fp, "%" PRIu64 "\n", T_bh[i]);
        avg1 += T_bh[i];
        if (i==9) fprintf(fp, "Avg. Time = %d\n\n", avg1/10); 
    }

    
    fprintf(fp, "--------------------------\n");
    fprintf(fp, "|  Time after cache hit  |\n");
    fprintf(fp, "--------------------------\n");

    for (int i=0; i<10; i++)
    {
        fprintf(fp, "%" PRIu64 "\n", T_ah[i]);
        avg2 += T_ah[i];
        if (i==9) fprintf(fp, "Avg. Time = %d", avg2/10); 
    }

    close(fd);
}


