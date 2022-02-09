#include <inttypes.h>
#include <x86intrin.h> 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#define MAXLOOP 50

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

const char* opcodes[13] = {
    "\x48\x92",
    "\x0f\x1f\x04\x1b",
    "\x66\x0f\x1f\x04\x1b",
    "\x48\x0f\xb1\x1c\x24",
    "\x0f\xae\xe8",
    "\x0f\x31",
    "\x0f\xa2",
    "\x66\x0f\x3a\x44\xc8\x11",
    "\x66\x0f\x38\xde\xc1",
    "\x66\x0f\x38\xdd\xc1",
    "\x66\x0f\x38\xdf\xc1",
    "\x66\x0f\x3a\xdf\xc8\x11",
    "\x66\x0f\x38\xdc\xc1"
};

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
    uint64_t T_bh[MAXLOOP], T_ah[MAXLOOP], T_bclf[MAXLOOP];

    
    
    /* before clflush */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < MAXLOOP; i++) {

       t1 = rdtsc_begin();

        __asm__ volatile(
            ".intel_syntax noprefix;"
            "mov rbx, 0x567890;"
            "push rbx;"
            "movsd xmm1, [rsp];"
            "mov rbx, 0x123456;"
            "push rbx;"
            "movsd xmm0, [rsp];"
            "pclmulqdq xmm1, xmm0, 0x11;"
            "aesenc xmm0, xmm1;"
            "aesenclast xmm0, xmm1;"
            "aesdec xmm0, xmm1;"
            "aesdeclast xmm0, xmm1;"
            "aeskeygenassist xmm1, xmm0, 0x11;"
            );

        t2 = rdtsc_end();

        T_bclf[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB before clflush-> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE before clflush -> %lld\n", count);


    // Flushing instructions from uop cache
    for (int i=0; i < 7; i++) {
        _mm_clflush(&opcodes[i]);
    }


    
    /* 1st access */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < MAXLOOP; i++) {

       t1 = rdtsc_begin();

        __asm__ volatile(
            ".intel_syntax noprefix;"
            "mov rbx, 0x567890;"
            "push rbx;"
            "movsd xmm1, [rsp];"
            "mov rbx, 0x123456;"
            "push rbx;"
            "movsd xmm0, [rsp];"
            "pclmulqdq xmm1, xmm0, 0x11;"
            "aesenc xmm0, xmm1;"
            "aesenclast xmm0, xmm1;"
            "aesdec xmm0, xmm1;"
            "aesdeclast xmm0, xmm1;"
            "aeskeygenassist xmm1, xmm0, 0x11;"
            );

        t2 = rdtsc_end();

        T_bh[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB before CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE before CH -> %lld\n", count);
   

    /* Consecutive accesses */

    for (int i=0; i < 5000; i++) {
        __asm__ volatile(
            ".intel_syntax noprefix;"
            "mov rbx, 0x567890;"
            "push rbx;"
            "movsd xmm1, [rsp];"
            "mov rbx, 0x123456;"
            "push rbx;"
            "movsd xmm0, [rsp];"
            "pclmulqdq xmm1, xmm0, 0x11;"
            "aesenc xmm0, xmm1;"
            "aesenclast xmm0, xmm1;"
            "aesdec xmm0, xmm1;"
            "aesdeclast xmm0, xmm1;"
            "aeskeygenassist xmm1, xmm0, 0x11;"
            );
    }




    /* Last access */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < MAXLOOP; i++) {

        t1 = rdtsc_begin();

        __asm__ volatile(
            ".intel_syntax noprefix;"
            "mov rbx, 0x567890;"
            "push rbx;"
            "movsd xmm1, [rsp];"
            "mov rbx, 0x123456;"
            "push rbx;"
            "movsd xmm0, [rsp];"
            "pclmulqdq xmm1, xmm0, 0x11;"
            "aesenc xmm0, xmm1;"
            "aesenclast xmm0, xmm1;"
            "aesdec xmm0, xmm1;"
            "aesdeclast xmm0, xmm1;"
            "aeskeygenassist xmm1, xmm0, 0x11;"
            );

        t2 = rdtsc_end();

        T_ah[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB after CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE after CH -> %lld\n", count);
    

    /* Saving Data in a File */
    
//    int avg0 = 0;
//    int avg1 = 0; 
//    int avg2 = 0;

    FILE *fp1 = fopen("time_bclf.txt", "w");
    
//    fprintf(fp, "---------------------------\n");
//    fprintf(fp, "|  Time before clflush    |\n");
//    fprintf(fp, "---------------------------\n");
    
    for (int i=0; i<MAXLOOP; i++)
    {
        fprintf(fp1, "%" PRIu64 "\n", T_bclf[i]);
//        avg0 += T_bclf[i];
//        if (i==9) fprintf(fp, "Avg. Time = %d\n\n", avg0/10); 
    }
    
    

    FILE *fp2 = fopen("time_bh.txt", "w");

//    fprintf(fp, "---------------------------\n");
//    fprintf(fp, "|  Time before cache hit  |\n");
//    fprintf(fp, "---------------------------\n");
    
    for (int i=0; i<MAXLOOP; i++)
    {
        fprintf(fp2, "%" PRIu64 "\n", T_bh[i]);
//        avg1 += T_bh[i];
//        if (i==9) fprintf(fp, "Avg. Time = %d\n\n", avg1/10); 
    }

    
    FILE *fp3 = fopen("time_ah.txt", "w");
    
//    fprintf(fp, "--------------------------\n");
//    fprintf(fp, "|  Time after cache hit  |\n");
//    fprintf(fp, "--------------------------\n");

    for (int i=0; i<MAXLOOP; i++)
    {
        fprintf(fp3, "%" PRIu64 "\n", T_ah[i]);
//        avg2 += T_ah[i];
//        if (i==9) fprintf(fp, "Avg. Time = %d\n", avg2/10); 
    }

    fclose(fp1);
    fclose(fp2);
    fclose(fp3);
}


