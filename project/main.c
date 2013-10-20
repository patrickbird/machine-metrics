#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>
#include <sys/time.h>

double get_time()
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);
    //struct timeval t;
    //struct timezone tzp;
    //gettimeofday(&t, &tzp);

    //printf("Sec is %d\n", t.tv_sec);
    //printf("Nano is %d\n", t.tv_usec);

    return ts.tv_nsec;
}

static int SetProcessorAffinity(int * arguments)
{
    cpu_set_t cpu_set;

    CPU_ZERO(&cpu_set);
    CPU_SET(0, &cpu_set);

    sched_setaffinity(0, sizeof(cpu_set), &cpu_set);
}

static int SetPriority(int * arguments)
{
    int priority = arguments[0];
    
    printf("Attempting to set priority to %d\n", priority);
    
    return setpriority(PRIO_PROCESS, 0, priority);
} 

static inline uint64_t GetRdtscValue(void)
{
    unsigned int low, high;
    __asm__ volatile("rdtsc" : "=a" (low), "=d" (high));
    
    return (uint64_t)low | ((uint64_t)high << 32);
}

static void Initialize(int (* function)(int *), int * arguments, char * string)
{
    //int returnValue;

    //returnValue = function(arguments);
    
    if (function(arguments) != 0)
    {
        printf("%s error.\n", string);
        exit(-1);
    }
    else
    {
        printf("%s success.\n", string);
    }  
}  

int main()
{
	double time, time2;
    uint64_t rdtsc1, rdtsc2;
    int arguments[1];
    int returnValue;
    int i = 0;

	puts("Hello World.");

    Initialize(&SetProcessorAffinity, NULL, "Processor Affinity");
    
    arguments[0] = PRIO_MIN;
    Initialize(&SetPriority, arguments, "Priority");

	time = get_time();
    time2 = get_time();
    
    for (i = 0; i < 50; i++)
    {
    rdtsc1 = GetRdtscValue();
    rdtsc2 = GetRdtscValue();

	printf("The time is %ld\n", rdtsc1);
    printf("The time is %ld\n", rdtsc2); 
    printf("The diff is %ld\n", rdtsc2 - rdtsc1);
    }

    printf("The highest nice value is: %d\n", PRIO_MAX);
    printf("The lowest nice value is:  %d\n", PRIO_MIN);

	return 0;
}
