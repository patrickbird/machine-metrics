#include <stdio.h>
#include <stdint.h>
#include <time.h>
//#include <sys/resource.h>

static inline uint64_t RDTSC()
{
   unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
     return ((uint64_t)hi << 32) | lo;
}

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

int main()
{
	double time, time2;
    uint64_t rdtsc1, rdtsc2;

	puts("Hello World.");

	time = get_time();
    time2 = get_time();
    
    rdtsc1 = RDTSC();
    rdtsc2 = RDTSC();

	printf("The time is %ld\n", rdtsc1);
    printf("The time is %ld\n", rdtsc2); 
    printf("The diff is %ld\n", rdtsc2 - rdtsc1);

	return 0;
}
