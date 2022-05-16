#include "instr.h"
#include <inttypes.h>
#include <x86intrin.h> 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#define MAXLOOP 20

void convertStrToBytes(char *buf, unsigned char *value)
{
    char *pos = buf;

    for (size_t count = 0; count < sizeof value/sizeof *value; count++) {
        sscanf(pos, "%2hhx", &value[count]);
        pos += 2;
    }
}

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


static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
}

int main(int argc, char** argv)
{
    int arg_count;
    arg_count = argc - 2;

    unsigned char *val1, *val2, *val3, *val4, *val5;

    long len = strlen(argv[2]);
    val1 = malloc(sizeof(char) * len/2);
    convertStrToBytes(argv[2], val1);

    if (arg_count == 1)
    {
        val2 = malloc(sizeof(char));
        val3 = malloc(sizeof(char));
        val4 = malloc(sizeof(char));
        val5 = malloc(sizeof(char));
        *val2 = '\x90';
        *val3 = '\x90';
        *val4 = '\x90';
        *val5 = '\x90';
    }

    if (arg_count > 1)
    {
        len = strlen(argv[3]);
        val2 = malloc(sizeof(char) * len/2);
        convertStrToBytes(argv[3], val2);

        if (arg_count == 2)
        {
            val3 = malloc(sizeof(char));
            val4 = malloc(sizeof(char));
            val5 = malloc(sizeof(char));
            *val3 = '\x90';
            *val4 = '\x90';
            *val5 = '\x90';
        }
    }
    else if (arg_count > 2)
    {
        len = strlen(argv[4]);
        val3 = malloc(sizeof(char) * len/2);
        convertStrToBytes(argv[4], val3);

        if (arg_count == 3)
        {
            val4 = malloc(sizeof(char));
            val5 = malloc(sizeof(char));
            *val4 = '\x90';
            *val5 = '\x90';
        }
    }
    else if (arg_count > 3)
    {
        len = strlen(argv[5]);
        val4 = malloc(sizeof(char) * len/2);
        convertStrToBytes(argv[5], val4);

        if (arg_count == 4)
        {
            val5 = malloc(sizeof(char));
            *val5 = '\x90';
        }
    }
    else if (arg_count > 4)
    {
        len = strlen(argv[6]);
        val5 = malloc(sizeof(char) * len/2);
        convertStrToBytes(argv[6], val5);
    }


    const char* opcodes[11] = {
        "\x48\xc7\xc1\x42\x42\x42\x00",
        "\x48\xc7\xc3\x41\x41\x41\x00",
        "\x68\x66\x66\x66\x00",
        "\xf2\x0f\x10\x0c\x24",
        "\x68\x44\x44\x44\x00",
        "\xf2\x0f\x10\x04\x24",
        val1,
        val2,
        val3,
        val4,
        val5
    };

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

        INSTR_SET;

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

        INSTR_SET;

        t2 = rdtsc_end();

        T_bh[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB before CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE before CH -> %lld\n", count);
   

    /* Consecutive accesses */

    for (int i=0; i < 100; i++) {
        INSTR_SET;
    }



    /* Last access */

    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    for (int i=0; i < MAXLOOP; i++) {

        t1 = rdtsc_begin();

        INSTR_SET;

        t2 = rdtsc_end();

        T_ah[i] = t2 - t1;
    }


    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    read(fd, &count, sizeof(long long));
    if (pe.config == 0x0879) printf("No. of UOPS from DSB after CH -> %lld\n", count);
    else if (pe.config == 0x0479) printf("No. of UOPS from MITE after CH -> %lld\n", count);
    

    /* Saving Data in a File */

    int sl_no;
    sl_no = atoi(argv[1]); 
    char fn1[20], fn2[20], fn3[20];
    
    
    snprintf(fn1, sizeof(fn1), "time_bclf_%d.txt", sl_no);
    FILE *fp1 = fopen(fn1, "w");
    
//    fprintf(fp, "---------------------------\n");
//    fprintf(fp, "|  Time before clflush    |\n");
//    fprintf(fp, "---------------------------\n");
    
    for (int i=0; i<MAXLOOP; i++)
    {
        fprintf(fp1, "%" PRIu64 "\n", T_bclf[i]);
//        avg0 += T_bclf[i];
//        if (i==9) fprintf(fp, "Avg. Time = %d\n\n", avg0/10); 
    }
    
    

    snprintf(fn2, sizeof(fn2), "time_bh_%d.txt", sl_no);
    FILE *fp2 = fopen(fn2, "w");

//    fprintf(fp, "---------------------------\n");
//    fprintf(fp, "|  Time before cache hit  |\n");
//    fprintf(fp, "---------------------------\n");
    
    for (int i=0; i<MAXLOOP; i++)
    {
        fprintf(fp2, "%" PRIu64 "\n", T_bh[i]);
//        avg1 += T_bh[i];
//        if (i==9) fprintf(fp, "Avg. Time = %d\n\n", avg1/10); 
    }

    
    snprintf(fn3, sizeof(fn3), "time_ah_%d.txt", sl_no);
    FILE *fp3 = fopen(fn3, "w");
    
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


