#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

static long
perf_event_open(struct perf_event_attr *hw_event, pid_t pid,
                int cpu, int group_fd, unsigned long flags)
{
    int ret;

    ret = syscall(__NR_perf_event_open, hw_event, pid, cpu,
                   group_fd, flags);
    return ret;
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

int
main(int argc, char **argv)
{
    struct perf_event_attr pe;
    long long count;
    int fd;

    uint64_t t1, t2, T_1st;
    
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = 0x0879;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }


    uint64_t T1[1000];
    long long uops[10];

   
    /* 1st access */
//    t1 = rdtsc_begin();
//
//    __asm__ volatile(
//       ".intel_syntax noprefix;"
//       "push rax;"
//       "push rbx;"
//       "cmpxchg [rsp], rbx;"
//       "push rdx;"
//       "cmpxchg [rsp], rdx;"
//       "div rbx;"
//       "xchg rax, rbx;"
//       "pop rdx;"
//       "pop rbx;"
//       "pop rax;");
//
//    t2 = rdtsc_end();
//
//    T_1st = t2 - t1;
    
    /* Cache hit */
    for (int i=0; i < 10; i++) 
    {
        ioctl(fd, PERF_EVENT_IOC_RESET, 0);
        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        t1 = rdtsc_begin();

        __asm__ volatile(
           ".intel_syntax noprefix;"
           "push rax;"
           "push rbx;"
           "cmpxchg [rsp], rbx;"
           "push rdx;"
           "cmpxchg [rsp], rdx;"
           "div rbx;"
           "xchg rax, rbx;"
           "pop rdx;"
           "pop rbx;"
           "pop rax;");

        t2 = rdtsc_end();


        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        read(fd, &count, sizeof(long long));
        uops[i] = count;
        
        T1[i] = t2 - t1;


        __asm__ volatile(
                ".intel_syntax noprefix;"
                "nop dword ptr [rax];"
                "nop dword ptr [rax + 0x0];"
                "nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop;"
                "nop; nop; nop; nop; nop;"
                "nop dword ptr [rax];"
                "nop dword ptr [rax + 0x0];"
                "nop dword ptr [rax];"
                "nop dword ptr [rax + 0x0];");
                
    }

//    printf("Time for 1st access : %" PRIu64 "\n", T_1st);

    printf("LOOP COMPLETED\n");

    FILE *fp = fopen("time_for_cache_hit.txt", "w");
    for (int i=0; i<10; i++) 
    {
        fprintf(fp, "%" PRIu64 " --  %lld \n", T1[i], uops[i]);
    }
    fclose(fp);

    printf("DATA COPIED TO A FILE\n");


    //printf("Number of UOPS -> %lld\n", count);

    close(fd);
}


