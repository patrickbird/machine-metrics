#define _GNU_SOURCE

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>

#include "metric.h"

static struct METRIC
{
    int SampleCount;
    int MinIndex;
    int MaxIndex;
    uint64_t First;
    uint64_t Min;
    uint64_t Max;
    double Mean;
    double StandardDeviation;
    double Sum;
    uint64_t * Samples;
    const char * Name;
    uint64_t (* Measure)(int *);
    int * Arguments;
} _metrics[MEASUREMENT_COUNT];

static const char * MetricNames[MEASUREMENT_COUNT] =
{
    "RDTSCP",
    "LOOP",
    "Procedure (Zero Arguments)",
    "Procedure (One Argument)",
    "Procedure (Two Arguments)",
    "Procedure (Three Arguments)",
    "Procedure (Four Arguments)",
    "Procedure (Five Arguments)",
    "Procedure (Six Arguments)",
    "Procedure (Seven Arguments)",
    "System Call",
    "Fork",
    "PThread",
    "Fork Context Switch",
    "PThread Context Switch"
};

static int _dummy;

static pthread_mutex_t _mutex;
static pthread_cond_t _condition;

static void UpdateMetric(enum MEASUREMENT measurement, int index);
static void UpdateMeasurementCalculations(enum MEASUREMENT measurement);
static inline void GetRdtscpValue(unsigned int * low, unsigned int * high);
static inline uint64_t GetUint64Value(unsigned int low, unsigned int high);

static uint64_t MeasureRdtscp(int * arguments);
static uint64_t MeasureLoop(int * arguments);
static uint64_t MeasureProcedureCall(int * arguments);
static uint64_t MeasureSystemCall(int * arguments);
static uint64_t MeasureZeroArguments(void);
static uint64_t MeasureOneArgument(int one);
static uint64_t MeasureTwoArguments(int one, int two);
static uint64_t MeasureThreeArguments(int one, int two, int three);
static uint64_t MeasureFourArguments(int one, int two, int three, int four);
static uint64_t MeasureFiveArguments(int one, int two, int three, int four, int five);
static uint64_t MeasureSixArguments(int one, int two, int three, int four, int five, int six);
static uint64_t MeasureSevenArguments(int one, int two, int three, int four, int five, int six, int seven);
static uint64_t MeasureFork(int * arguments);
static uint64_t MeasurePthread(int * arguments);
static uint64_t MeasureForkContextSwitch(int * arguments);
static uint64_t MeasurePthreadContextSwitch(int * arguments);

extern void InitializeMetrics(int sampleCount)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        _metrics[i].Max = 0;
        _metrics[i].Min = ULLONG_MAX;
        _metrics[i].Sum = 0;
        _metrics[i].SampleCount = sampleCount;
        _metrics[i].Samples = calloc(_metrics[i].SampleCount, sizeof(double));
        _metrics[i].Name = MetricNames[i];
        _metrics[i].Arguments = NULL;
    }

    _metrics[RDTSCP].Measure = MeasureRdtscp;
    _metrics[LOOP].Measure = MeasureLoop;
    _metrics[SYSTEM_CALL].Measure = MeasureSystemCall;
    _metrics[FORK].Measure = MeasureFork;
    _metrics[PTHREAD].Measure = MeasurePthread;
    _metrics[FORK_CONTEXT_SWITCH].Measure = MeasureForkContextSwitch;
    _metrics[PTHREAD_CONTEXT_SWITCH].Measure = MeasurePthreadContextSwitch;

    for (i = PROCEDURE_INITIAL; i <= PROCEDURE_FINAL; i++)
    {
        _metrics[i].Measure = MeasureProcedureCall;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = i;
    }

    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_condition, NULL);
}

extern void FinalizeMetrics(void)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        free(_metrics[i].Samples);
    }

    for (i = PROCEDURE_INITIAL; i <= PROCEDURE_FINAL; i++)
    {
        free(_metrics[i].Arguments);
    }

    printf("Reference the dummy: %d\n", _dummy);
}

extern void PerformMeasurement(enum MEASUREMENT measurement)
{
    int i;
    double value;

    for (i = 0; i < _metrics[measurement].SampleCount; i++)
    {
        _metrics[measurement].Samples[i] =  _metrics[measurement].Measure(_metrics[measurement].Arguments);
        UpdateMetric(measurement, i);
    }

    UpdateMeasurementCalculations(measurement);

    printf("--------------------------------------\n");
    printf("%s\n", _metrics[measurement].Name);
    printf("--------------------------------------\n");
    printf("First: %ld\n", _metrics[measurement].First);
    printf("Min: %ld at count %d\n", _metrics[measurement].Min, _metrics[measurement].MinIndex);
    printf("Max: %ld at count %d\n", _metrics[measurement].Max, _metrics[measurement].MaxIndex);
    printf("Average: %f\n", _metrics[measurement].Mean);
    printf("Standard Deviation: %f\n", _metrics[measurement].StandardDeviation);
    printf("Sample Count: %d\n\n", _metrics[measurement].SampleCount);

    /*
    for (i = 0; i < _metrics[measurement].SampleCount; i++)
    {
        printf("%ld, ", _metrics[measurement].Samples[i]);
    }

    printf("\n");
    */
}

static void UpdateMetric(enum MEASUREMENT measurement, int index)
{
    uint64_t value = _metrics[measurement].Samples[index];

    if (index == 0)
    {
        _metrics[measurement].First = value;
    }
    else
    {
        if (value < _metrics[measurement].Min)
        {
            _metrics[measurement].Min = value;
            _metrics[measurement].MinIndex = index;
        }
    
        if (value > _metrics[measurement].Max)
        {
            _metrics[measurement].Max = value;
            _metrics[measurement].MaxIndex = index;
        }
    }

    _metrics[measurement].Sum += value;
}

static void UpdateMeasurementCalculations(enum MEASUREMENT measurement)
{
    int i;
    double sum;
    double average;
    double difference;

    average = _metrics[measurement].Sum / _metrics[measurement].SampleCount;
    
    for (i = 0; i < _metrics[measurement].SampleCount; i++)
    {
        difference = _metrics[measurement].Samples[i] - average;
        sum += (difference * difference);
    }

    _metrics[measurement].Mean = average;
    _metrics[measurement].StandardDeviation = sqrt(sum / _metrics[measurement].SampleCount);
}

static inline void GetRdtscpValue(unsigned int * low, unsigned int * high)
{
    uint32_t temp;
    __asm__ volatile("rdtscp" : "=a" (*low), "=d" (*high), "=c"(temp): : );
}

static inline uint64_t GetUint64Value(unsigned int low, unsigned int high)
{
    return (uint64_t)low | ((uint64_t)high << 32);
}

static uint64_t MeasureRdtscp(int * arguments)
{
    unsigned int rdtsc1Low, rdtsc1High;
    unsigned int rdtsc2Low, rdtsc2High;

    GetRdtscpValue(&rdtsc1Low, &rdtsc1High);
    GetRdtscpValue(&rdtsc2Low, &rdtsc2High);

    return GetUint64Value(rdtsc2Low, rdtsc2High) - GetUint64Value(rdtsc1Low, rdtsc1High);
}

static uint64_t MeasureLoop(int * arguments)
{
    int i = 0;
    int sum = 0;
    unsigned int lows[100];
    unsigned int highs[100];

    for (i = 0; i < 100; i++)
    {
        GetRdtscpValue(&lows[i], &highs[i]);
    }

    for (i = 0; i < 99; i++)
    {
        sum += ((GetUint64Value(lows[i + 1], highs[i + 1]) - GetUint64Value(lows[i], highs[i]))); 
    }

    return (uint64_t)(sum / 99.0);
}

static uint64_t MeasureSevenArguments(int one, int two, int three, int four, int five, int six, int seven)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);
    
    _dummy = one + two + three + four + five + six + seven;

    return GetUint64Value(low, high);
}

static uint64_t MeasureSixArguments(int one, int two, int three, int four, int five, int six)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);
    
    _dummy = one + two + three + four + five + six;

    return GetUint64Value(low, high);
}

static uint64_t MeasureFiveArguments(int one, int two, int three, int four, int five)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    _dummy = one + two + three + four + five;

    return GetUint64Value(low, high);
}

static uint64_t MeasureFourArguments(int one, int two, int three, int four)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    _dummy = one + two + three + four;
    
    return GetUint64Value(low, high);
}

static uint64_t MeasureThreeArguments(int one, int two, int three)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);
    
    _dummy = one + two + three;

    return GetUint64Value(low, high);
}

static uint64_t MeasureTwoArguments(int one, int two)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    _dummy = one + two;

    return GetUint64Value(low, high);
}

static uint64_t MeasureOneArgument(int one)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    _dummy = one;

    return GetUint64Value(low, high);
}

static uint64_t MeasureZeroArguments(void)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureProcedureCall(int * arguments)
{
    unsigned int low, high;
    uint64_t ticks;
    int one, two, three, four, five, six, seven;

    switch (arguments[0] - PROCEDURE_INITIAL)
    {
        case 0:
            GetRdtscpValue(&low, &high);
            ticks = MeasureZeroArguments();
            break;

        case 1:
            GetRdtscpValue(&low, &high);
            ticks = MeasureOneArgument(one);
            break;

        case 2:
            GetRdtscpValue(&low, &high);
            ticks = MeasureTwoArguments(one, two);
            break;

        case 3:
            GetRdtscpValue(&low, &high);
            ticks = MeasureThreeArguments(one, two, three);
            break;

        case 4:
            GetRdtscpValue(&low, &high);
            ticks = MeasureFourArguments(one, two, three, four);
            break;

        case 5:
            GetRdtscpValue(&low, &high);
            ticks = MeasureFiveArguments(one, two, three, four, five);
            break;

        case 6:
            GetRdtscpValue(&low, &high);
            ticks = MeasureSixArguments(one, two, three, four, five, six);
            break;

        case 7:
            GetRdtscpValue(&low, &high);
            ticks = MeasureSevenArguments(one, two, three, four, five, six, seven);
            break;
    }

    ticks = (ticks - GetUint64Value(low, high));
    return ticks;
}

static uint64_t MeasureSystemCall(int * arguments)
{
    pid_t pid;
    unsigned int low1, high1, low2, high2;

    GetRdtscpValue(&low1, &high1);

    pid = getpid();

    GetRdtscpValue(&low2, &high2);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureFork(int * arguments)
{
    pid_t pid;
    unsigned int low1, high1, low2, high2;

    GetRdtscpValue(&low1, &high1);

    pid = fork();

    if (pid == 0)
    {
        exit(0);
    }
    else
    {
        GetRdtscpValue(&low2, &high2);
    }

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static void * DoStuff(void * arguments)
{
    return NULL;
}


static void * HelpContextSwitch(void * arguments)
{
    unsigned int low, high;
    uint64_t * value = calloc(1, sizeof(uint64_t));

    GetRdtscpValue(&low, &high);
    
    pthread_cond_signal(&_condition);
    
    *value =  GetUint64Value(low, high);

    pthread_exit(value);
}

static uint64_t MeasurePthread(int * arguments)
{
    int retValue;
    unsigned int low1, high1, low2, high2;
    pthread_t thread;

    GetRdtscpValue(&low1, &high1);

    retValue = pthread_create(&thread, NULL, DoStuff, NULL);

    GetRdtscpValue(&low2, &high2);
    
    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasurePthreadContextSwitch(int * arguments)
{
    int retValue;
    unsigned int low, high;
    pthread_t thread1;
    uint64_t * time1;
    uint64_t difference;

    retValue = pthread_create(&thread1, NULL, HelpContextSwitch, "One");
    
    pthread_cond_wait(&_condition, &_mutex);
    
    GetRdtscpValue(&low, &high);

    pthread_join(thread1, (void**)&time1);

    difference = GetUint64Value(low, high) - time1[0];
    
    free(time1);

    return difference;    
}

static void HandleSigusr1(int signal)
{
    return;
}

static uint64_t MeasureForkContextSwitch(int * arguments)
{
    pid_t pid, parentPid;
    unsigned int low1, high1, low2, high2;
    int desc[2];

    pipe(desc);

    parentPid = getpid(); 
    pid = fork();

    if (pid == 0)
    {   
        int retValue;
        int descriptor;
        uint64_t startTime;

        close(desc[0]);
        GetRdtscpValue(&low1, &high1);
        startTime = GetUint64Value(low1, high1);
        
        retValue = kill(parentPid, SIGUSR1);

        write(desc[1], &startTime, sizeof(startTime));

        exit(retValue);
    }
    else
    {
        int descriptor;
        uint64_t startTime;

        close(desc[1]);

        signal(SIGUSR1, HandleSigusr1);
        pause();
       
        GetRdtscpValue(&low2, &high2);
    
        read(desc[0], &startTime, sizeof(startTime));
        
        return GetUint64Value(low2, high2) - startTime;
    }
}

static uint64_t MeasureMainMemory(int * arguments)
{
    unsigned int low1, high1, low2, high2;
    int * largeBlock = malloc(1000000);
    int dummy;
    int index = rand() % 1000000;

    GetRdtscpValue(&low1, &high1);

    dummy = largeBlock[index];

    GetRdtscpValue(&low2, &high2);    

    dummy++;

    free(largeBlock);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureL1Cache(int * arguments)
{
    const BLOCK_SIZE = 1024;

    unsigned int low1, high1, low2, high2;
    int i, dummy, index;
    int * largeBlock = calloc(BLOCK_SIZE, sizeof(int));

    index = rand() % BLOCK_SIZE;

    for (i = 0; i < BLOCK_SIZE; i++)
    {
        dummy += largeBlock[i];
    }

    GetRdtscpValue(&low1, &high1);

    dummy = largeBlock[index];

    GetRdtscpValue(&low2, &high2);

    dummy++;

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);   
}


