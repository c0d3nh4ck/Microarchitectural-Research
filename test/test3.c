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

    uint64_t t1, t2;
    uint64_t T1, T2, T3;
    
    memset(&pe, 0, sizeof(struct perf_event_attr));
    pe.type = PERF_TYPE_RAW;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = 0x0479;
    pe.disabled = 1;
    pe.exclude_kernel = 1;
//    pe.exclude_hv = 1;

    fd = perf_event_open(&pe, 0, -1, -1, 0);
    if (fd == -1) {
       fprintf(stderr, "Error opening leader %llx\n", pe.config);
       exit(EXIT_FAILURE);
    }

    //ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    //ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

    t1 = rdtsc_begin();

    __asm__ volatile(
       ".intel_syntax noprefix;"
       "push rax;"
       "push rbx;"
       "push rcx;"
       "mov rcx, 0xdb;"
       "mov rbx, 0x75;"
       "pop rcx;"
       "pop rbx;"
       "pop rax;");

    t2 = rdtsc_end();

    T1 = t2 - t1;


    /* 2nd ACCESS */

    t1 = rdtsc_begin();

    __asm__ volatile(
        ".intel_syntax noprefix;"
       "push rax;"
       "push rbx;"
       "push rcx;"
       "mov rcx, 0xdb;"
       "mov rbx, 0x75;"
       "pop rcx;"
       "pop rbx;"
       "pop rax;");

    t2 = rdtsc_end();

    T2 = t2 - t1;


    /* 3rd ACCESS */

    t1 = rdtsc_begin();

    __asm__ volatile(
        ".intel_syntax noprefix;"
       "push rax;"
       "push rbx;"
       "push rcx;"
       "mov rcx, 0xdb;"
       "mov rbx, 0x75;"
       "pop rcx;"
       "pop rbx;"
       "pop rax;");

    t2 = rdtsc_end();

    T3 = t2 - t1;


    printf("Time for 1st access : %" PRIu64 "\n", T1);
    printf("Time for 2nd access : %" PRIu64 "\n", T2);
    printf("Time for 3rd access : %" PRIu64 "\n", T3);


    //ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    //read(fd, &count, sizeof(long long));

    //printf("Number of UOPS -> %lld\n", count);

    close(fd);
}


