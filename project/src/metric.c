#define _GNU_SOURCE

#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <netinet/in.h>
#include <netinet/ip.h>

#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

#include "metric.h"

#define BLOCK_COUNT 5000
#define ONE_KB 1024
#define ONE_MB (ONE_KB * ONE_KB)
#define BLOCK_INDEX_COUNT 10
#define PAGE_SIZE (4 * ONE_KB)
#define ONE_GB (256 * ONE_KB * ONE_KB)

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

struct Node
{
    char Data[500];
    struct Node * Next;
};

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
    "PThread Context Switch",
    
    "Memory 10 S1",
    "Memory 11 S1",
    "Memory 12 S1",
    "Memory 13 S1",
    "Memory 14 S1",
    "Memory 15 S1",
    "Memory 16 S1",
    "Memory 17 S1",
    "Memory 18 S1",
    "Memory 19 S1",
    "Memory 20 S1",
    "Memory 21 S1",
    "Memory 22 S1",
    "Memory 23 S1",
    "Memory 24 S1",
    "Memory 25 S1",
    "Memory 26 S1",
    "Memory 27 S1",
    "Memory 28 S1",
    "Memory 29 S1",

    "Memory 10 S2",
    "Memory 11 S2",
    "Memory 12 S2",
    "Memory 13 S2",
    "Memory 14 S2",
    "Memory 15 S2",
    "Memory 16 S2",
    "Memory 17 S2",
    "Memory 18 S2",
    "Memory 19 S2",
    "Memory 20 S2",
    "Memory 21 S2",
    "Memory 22 S2",
    "Memory 23 S2",
    "Memory 24 S2",
    "Memory 25 S2",
    "Memory 26 S2",
    "Memory 27 S2",
    "Memory 28 S2",
    "Memory 29 S2",

    "Memory 10 S3",
    "Memory 11 S3",
    "Memory 12 S3",
    "Memory 13 S3",
    "Memory 14 S3",
    "Memory 15 S3",
    "Memory 16 S3",
    "Memory 17 S3",
    "Memory 18 S3",
    "Memory 19 S3",
    "Memory 20 S3",
    "Memory 21 S3",
    "Memory 22 S3",
    "Memory 23 S3",
    "Memory 24 S3",
    "Memory 25 S3",
    "Memory 26 S3",
    "Memory 27 S3",
    "Memory 28 S3",
    "Memory 29 S3",

    "Memory 10 S4",
    "Memory 11 S4",
    "Memory 12 S4",
    "Memory 13 S4",
    "Memory 14 S4",
    "Memory 15 S4",
    "Memory 16 S4",
    "Memory 17 S4",
    "Memory 18 S4",
    "Memory 19 S4",
    "Memory 20 S4",
    "Memory 21 S4",
    "Memory 22 S4",
    "Memory 23 S4",
    "Memory 24 S4",
    "Memory 25 S4",
    "Memory 26 S4",
    "Memory 27 S4",
    "Memory 28 S4",
    "Memory 29 S4",

    "Memory 10 S5",
    "Memory 11 S5",
    "Memory 12 S5",
    "Memory 13 S5",
    "Memory 14 S5",
    "Memory 15 S5",
    "Memory 16 S5",
    "Memory 17 S5",
    "Memory 18 S5",
    "Memory 19 S5",
    "Memory 20 S5",
    "Memory 21 S5",
    "Memory 22 S5",
    "Memory 23 S5",
    "Memory 24 S5",
    "Memory 25 S5",
    "Memory 26 S5",
    "Memory 27 S5",
    "Memory 28 S5",
    "Memory 29 S5",

    "L1 Cache",
    "L2 Cache",
    "Back-to-back Latency",
    "RAM Write Bandwidth",
    "RAM Read Bandwidth",
    "Page Fault",
    
    "TCP Round Trip (Loopback)",
    "TCP Bandwidth (Loopback)",
    "TCP Setup (Loopback)",
    "TCP Teardown (Loopback)",
    
    "TCP Round Trip (Remote)",
    "TCP Bandwidth (Remote)",
    "TCP Setup (Remote)",
    "TCP Teardown (Remote)",

    "File Cache (1 MB)",
    "File Cache (2 MB)",
    "File Cache (4 MB)",
    "File Cache (8 MB)",
    "File Cache (16 MB)",
    "File Cache (32 MB)",
    "File Cache (64 MB)",
    "File Cache (128 MB)",
    "File Cache (256 MB)",
    "File Cache (512 MB)",
    "File Cache (1 GB)",
    "File Cache (2 GB)",
    "File Cache (4 GB)",
    "File Cache (8 GB)",
    "File Cache (16 GB)",
    "File Cache (32 GB)",

    "Local File Read (1 MB)",
    "Local File Read (2 MB)",
    "Local File Read (4 MB)",
    "Local File Read (8 MB)",
    "Local File Read (16 MB)",
    "Local File Read (32 MB)",
    "Local File Read (64 MB)",
    "Local File Read (128 MB)",
    "Local File Read (256 MB)",
    "Local File Read (512 MB)",
    "Local File Read (1 GB)",
    //"Local File Read (2 GB)",
    //"Local File Read (4 GB)",
    //"Local File Read (8 GB)",
    //"Local File Read (16 GB)",
    //"Local File Read (32 GB)",

    "Local File Random Read (1 MB)",
    "Local File Random Read (2 MB)",
    "Local File Random Read (4 MB)",
    "Local File Random Read (8 MB)",
    "Local File Random Read (16 MB)",
    "Local File Random Read (32 MB)",
    "Local File Random Read (64 MB)",
    "Local File Random Read (128 MB)",
    "Local File Random Read (256 MB)",
    "Local File Random Read (512 MB)",
    "Local File Random Read (1 GB)",
    //"Local File Random Read (2 GB)",
    //"Local File Random Read (4 GB)",
    //"Local File Random Read (8 GB)",
    //"Local File Random Read (16 GB)",
    //"Local File Random Read (32 GB)",

    "Remote File Read (1 MB)",
    "Remote File Read (2 MB)",
    "Remote File Read (4 MB)",
    "Remote File Read (8 MB)",
    "Remote File Read (16 MB)",
    "Remote File Read (32 MB)",
    "Remote File Read (64 MB)",
    "Remote File Read (128 MB)",
    "Remote File Read (256 MB)",
    "Remote File Read (512 MB)",
    "Remote File Read (1 GB)",
    //"Remote File Read (2 GB)",
    //"Remote File Read (4 GB)",
    //"Remote File Read (8 GB)",
    //"Remote File Read (16 GB)",
    //"Remote File Read (32 GB)",

    "Remote File Random Read (1 MB)",
    "Remote File Random Read (2 MB)",
    "Remote File Random Read (4 MB)",
    "Remote File Random Read (8 MB)",
    "Remote File Random Read (16 MB)",
    "Remote File Random Read (32 MB)",
    "Remote File Random Read (64 MB)",
    "Remote File Random Read (128 MB)",
    "Remote File Random Read (256 MB)",
    "Remote File Random Read (512 MB)",
    "Remote File Random Read (1 GB)",
    //"Remote File Random Read (2 GB)",
    //"Remote File Random Read (4 GB)",
    //"Remote File Random Read (8 GB)",
    //"Remote File Random Read (16 GB)",
    //"Remote File Random Read (32 GB)",
    
    "Contention (1 Thread)",
    "Contention (2 Threads)",
    "Contention (3 Threads)",
    "Contention (4 Threads)",
    "Contention (5 Threads)",
    "Contention (6 Threads)",
    "Contention (7 Threads)",
    "Contention (8 Threads)",
    "Contention (9 Threads)",
    "Contention (10 Threads)",
    "Contention (11 Threads)",
    "Contention (12 Threads)",
    "Contention (13 Threads)",
    "Contention (14 Threads)",
    "Contention (15 Threads)",
    "Contention (16 Threads)",
    "Contention (17 Threads)",
    "Contention (18 Threads)",
    "Contention (19 Threads)",
    "Contention (20 Threads)",

    "Contention (21 Thread)",
    "Contention (22 Threads)",
    "Contention (23 Threads)",
    "Contention (24 Threads)",
    "Contention (25 Threads)",
    "Contention (26 Threads)",
    "Contention (27 Threads)",
    "Contention (28 Threads)",
    "Contention (29 Threads)",
    "Contention (30 Threads)",
    "Contention (31 Threads)",
    "Contention (32 Threads)",
    "Contention (33 Threads)",
    "Contention (34 Threads)",
    "Contention (35 Threads)",
    "Contention (36 Threads)",
    "Contention (37 Threads)",
    "Contention (38 Threads)",
    "Contention (39 Threads)",
    "Contention (40 Threads)",

};

static int _dummy;
static struct Node  _headNode;
static int   _memory[ONE_GB];

static pthread_mutex_t _mutex;
static pthread_cond_t _condition;
static pthread_barrier_t * _barrier;

static void UpdateMetric(enum MEASUREMENT measurement, int index);
static void UpdateMeasurementCalculations(enum MEASUREMENT measurement);
static inline void GetRdtscpValue(unsigned int * low, unsigned int * high);
static inline uint64_t GetUint64Value(unsigned int low, unsigned int high);
static int GetPseudoRand(void);

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
static uint64_t MeasureMainMemory(int * arguments);
static uint64_t MeasureL1Cache(int * arguments);
static uint64_t MeasureL2Cache(int * arguments);
static uint64_t MeasureBackToBackLoad(int * arguments);
static uint64_t MeasureRamWrite(int * arguments);
static uint64_t MeasureRamRead(int * arguments);
static uint64_t MeasurePageFault(int * arguments);
static uint64_t MeasureTcpRoundTrip(int * arguments);
static uint64_t MeasureTcpBandwidth(int * arguments);
static uint64_t MeasureTcpSetup(int * arguments);
static uint64_t MeasureTcpTeardown(int * arguments);
static uint64_t MeasureFileCache(int * arguments);
static uint64_t MeasureFileRead(int * arguments);
static uint64_t MeasureFileRandomRead(int * arguments);
static uint64_t MeasureContention(int * arguments);

extern void InitializeMetrics(int sampleCount)
{
    int i;
    struct Node * node;
    int argument;
    int stride, size;
    int ipAddress;

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

    _metrics[L1_CACHE].Measure = MeasureL1Cache;
    _metrics[L2_CACHE].Measure = MeasureL2Cache;
    _metrics[BACK_TO_BACK].Measure = MeasureBackToBackLoad;
    _metrics[RAM_WRITE_BANDWIDTH].Measure = MeasureRamWrite;
    _metrics[RAM_READ_BANDWIDTH].Measure = MeasureRamRead;
    _metrics[PAGE_FAULT].Measure = MeasurePageFault;
    
    _metrics[TCP_ROUND_TRIP_LOOPBACK].Measure = MeasureTcpRoundTrip;
    _metrics[TCP_BANDWIDTH_LOOPBACK].Measure = MeasureTcpBandwidth;
    _metrics[TCP_SETUP_LOOPBACK].Measure = MeasureTcpSetup;
    _metrics[TCP_TEARDOWN_LOOPBACK].Measure = MeasureTcpTeardown;
    
    _metrics[TCP_ROUND_TRIP_REMOTE].Measure = MeasureTcpRoundTrip;
    _metrics[TCP_BANDWIDTH_REMOTE].Measure = MeasureTcpBandwidth;
    _metrics[TCP_SETUP_REMOTE].Measure = MeasureTcpSetup;
    _metrics[TCP_TEARDOWN_REMOTE].Measure = MeasureTcpTeardown;

    for (i = PROCEDURE_INITIAL; i <= PROCEDURE_FINAL; i++)
    {
        _metrics[i].Measure = MeasureProcedureCall;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = i;
    }

    stride = 256;
    
    for (i = 0; i < 5; i++)
    {
        int j;
        size = 64;

        for (j = MEM_INITIAL + 20 * i; j < MEM_INITIAL + 20 * i + 20; j++)
        {
            _metrics[j].Measure = MeasureMainMemory;
            _metrics[j].Arguments = calloc(2, sizeof(int));
            _metrics[j].Arguments[0] = stride;
            _metrics[j].Arguments[1] = size;

            size = size << 1;
        }

        stride = stride << 2;
    }

    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_condition, NULL);

    node = calloc(1, sizeof(struct Node));
    _headNode.Next = node;
    for (i = 0; i < 1000; i++)
    {
        node->Next = calloc(1, sizeof(struct Node));
        node = node->Next;
    }
    
    node->Next = NULL;

    inet_pton(AF_INET, "127.0.0.1", &ipAddress);
    for (i = TCP_ROUND_TRIP_LOOPBACK; i <= TCP_TEARDOWN_LOOPBACK; i++)
    {
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = ipAddress;
    }

    inet_pton(AF_INET, "10.12.232.33", &ipAddress);
    for (i = TCP_ROUND_TRIP_REMOTE; i <= TCP_TEARDOWN_REMOTE; i++)
    {
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = ipAddress;
    }

    for (i = FILE_CACHE_1MB; i <= FILE_CACHE_32GB; i++)
    {
        _metrics[i].Measure = MeasureFileCache;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = (int)pow(2, i - FILE_CACHE_1MB);
    }

    for (i = FILE_READ_1MB; i <= FILE_READ_1GB; i++)
    {
        _metrics[i].Measure = MeasureFileRead;
        _metrics[i].Arguments = calloc(2, sizeof(int));
        _metrics[i].Arguments[0] = (int)pow(2, i - FILE_READ_1MB);
        _metrics[i].Arguments[1] = 1; // is local
    }

    for (i = FILE_RANDOM_READ_1MB; i <= FILE_RANDOM_READ_1GB; i++)
    {
        _metrics[i].Measure = MeasureFileRandomRead;
        _metrics[i].Arguments = calloc(2, sizeof(int));
        _metrics[i].Arguments[0] = (int)pow(2, i - FILE_RANDOM_READ_1MB);
        _metrics[i].Arguments[1] = 1; // is local
    }

    for (i = REMOTE_FILE_READ_1MB; i <= REMOTE_FILE_READ_1GB; i++)
    {
        _metrics[i].Measure = MeasureFileRead;
        _metrics[i].Arguments = calloc(2, sizeof(int));
        _metrics[i].Arguments[0] = (int)pow(2, i - REMOTE_FILE_READ_1MB);
        _metrics[i].Arguments[1] = 0; // is local
    }

    for (i = REMOTE_FILE_RANDOM_READ_1MB; i <= REMOTE_FILE_RANDOM_READ_1GB; i++)
    {
        _metrics[i].Measure = MeasureFileRandomRead;
        _metrics[i].Arguments = calloc(2, sizeof(int));
        _metrics[i].Arguments[0] = (int)pow(2, i - REMOTE_FILE_RANDOM_READ_1MB);
        _metrics[i].Arguments[1] = 0; // is local
    }

    for (i = CONTENTION_THREAD_1; i <= CONTENTION_THREAD_40; i++)
    {
        _metrics[i].Measure = MeasureContention;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = i - CONTENTION_THREAD_1 + 1;
    }
}

extern void FinalizeMetrics(void)
{
    int i;
    char fileName[50];
    struct Node * node = _headNode.Next;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        free(_metrics[i].Samples);
    }

    for (i = PROCEDURE_INITIAL; i <= PROCEDURE_FINAL; i++)
    {
        free(_metrics[i].Arguments);
    }

    for (i = MEM_INITIAL; i <= MEM_FINAL; i++)
    {
        free(_metrics[i].Arguments);
    }

    //for (i = 0; i < ONE_GB; i++)
    //{
    //    free(_memory[i]);
    //}

    while (node->Next != NULL)
    {
        struct Node * temp = node->Next;
        free(node);
        node = temp;
    }

    system("rm /tmp/pagefault*");

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

        //if (measurement >= MEM_INITIAL && measurement <= MEM_FINAL)
        //{
        //    int stripes = _metrics[measurement].Arguments[0];
        //    int size = _metrics[measurement].Arguments[1];

        //    average = average - _metrics[RDTSCP].Mean;// - ((size/stripes) * _metrics[LOOP].Mean);
        //    average = average / ((double) size / (double)stripes);
        //}

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
    time_t unixTime;
    unsigned int low1, high1, low2, high2;

    GetRdtscpValue(&low1, &high1);

    time(&unixTime);

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

static int GetPseudoRand(void)
{
    srand(rand() ^ time(NULL));
    return rand();
}

static int GetPseudoRandomBlockNumber(void)
{
    static int previousIndicies[BLOCK_INDEX_COUNT] = {0};
    int i  = 0;
    int blockNumber;

    while (i != BLOCK_INDEX_COUNT)
    {
        blockNumber = GetPseudoRand() % BLOCK_COUNT;

        for (i = 0; i < BLOCK_INDEX_COUNT; i++)
        {
            if (abs(blockNumber - previousIndicies[i]) < 100)
            {
                break;
            }
        }
    }

    for (i = 0; i < BLOCK_INDEX_COUNT - 1; i++)
    {
        previousIndicies[i] = previousIndicies[i + 1];
    }

    previousIndicies[BLOCK_INDEX_COUNT - 1] = blockNumber;

    return blockNumber;
}

static void ClearCache(void)
{
    int i;

    for (i = 0; i < ONE_GB; i++)
    {
        _memory[i] = i;
    }
}

static uint64_t MeasureMainMemory(int * arguments)
{
    unsigned int low1, high1, low2, high2;
    int dummy;
    int stride = arguments[0];
    int size = arguments[1];
    int i;
    //int index = GetPseudoRand() % step;
    GetRdtscpValue(&low1, &high1);

    for (i = 0; i < size; i = i + stride)
    {
        dummy = _memory[i];
    } 

    GetRdtscpValue(&low2, &high2);    

    dummy++;

    return (GetUint64Value(low2, high2) - GetUint64Value(low1, high1)) / ((double)size / (double)stride);
}

static uint64_t MeasureBackToBackLoad(int * arguments)
{
    struct Node * node = _headNode.Next;
    unsigned int low1, high1, low2, high2;
    int i, j;

    GetRdtscpValue(&low1, &high1);

    while (node->Next != NULL)
    {
        node = node->Next;
    }

    GetRdtscpValue(&low2, &high2);    

    ClearCache();

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureL1Cache(int * arguments)
{
    const int INT_COUNT = 50;

    unsigned int low1, high1, low2, high2;
    int i;
    //int start = GetPseudoRand() % ONE_GB - INT_COUNT; //GetPseudoRandomBlockNumber();

    for (i = 0; i < INT_COUNT; i++)
    {
        _memory[i] = 0x55AA55AA;//GetPseudoRand() % INT_MAX;
    }

    GetRdtscpValue(&low1, &high1);

    _dummy = _memory[(INT_COUNT >> 1)];

    GetRdtscpValue(&low2, &high2);

    ClearCache();

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);   
}

static uint64_t MeasureL2Cache(int * arguments)
{
    const int BLOCK_LENGTH = 50;

    unsigned int low1, high1, low2, high2;
    int i, index;

    // Read into L1 and spill into L2
    for (i = 0; i < 20000; i++)
    {
        _memory[i] = GetPseudoRand() % INT_MAX;
    }
    
    index = GetPseudoRand() % 1000; 

    GetRdtscpValue(&low1, &high1);

    _dummy = _memory[index];

    GetRdtscpValue(&low2, &high2);

    ClearCache();

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureRamWrite(int * arguments)
{
    unsigned int low1, high1, low2, high2;
    int i, j;

    GetRdtscpValue(&low1, &high1);

    asm("cld\n"
        "rep stosq"
        : : "D" (_memory), "c" (sizeof(_memory) >> 3), "a" (0) );

    GetRdtscpValue(&low2, &high2);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureRamRead(int * arguments)
{
    unsigned int low1, high1, low2, high2;
    int i, j;

    GetRdtscpValue(&low1, &high1);

    asm("cld\n"
        "rep lodsq"
        : : "S" (_memory), "c" (sizeof(_memory) >> 3) : "%eax");

    GetRdtscpValue(&low2, &high2);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasurePageFault(int * arguments)
{
    FILE * file;
    unsigned int low1, low2, high1, high2;
    int dummy;
    uint64_t seekPos;    
    uint64_t diff;

    file = fopen("bigfile", "r");

    if (file == NULL)
    {
        printf("Couldn't open file.\n");
        exit(0);
    }

    seekPos = (((uint64_t)GetPseudoRand() << 32) | (uint64_t)GetPseudoRand()) % 300000000000ULL;

    GetRdtscpValue(&low1, &high1);
    
    fseek(file, seekPos, SEEK_SET);

    fread(&dummy, sizeof(int), 1, file);

    GetRdtscpValue(&low2, &high2);

    _dummy = dummy + 1;

    diff =  GetUint64Value(low2, high2) - GetUint64Value(low1, high1);


    fclose(file);

    return diff;
}


static uint64_t MeasureTcpRoundTrip(int * arguments)
{
    int descriptor;
    struct sockaddr_in server;
    char * txMessage = "Hello World";
    char rxMessage[100];
    unsigned int low1, low2, high1, high2;
    int bufferLength = strlen(txMessage);
    int i;

    descriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_family = AF_INET;
    server.sin_port = htons(7);
    server.sin_addr.s_addr = arguments[0];

    connect(descriptor, (const struct sockaddr *)&server, sizeof(server));
   
    GetRdtscpValue(&low1, &high1);
    
    for (i = 0; i < 1000; i++)
    {
        send(descriptor, txMessage, bufferLength, 0);

        recv(descriptor, rxMessage, bufferLength, 0); 
    }

    GetRdtscpValue(&low2, &high2);

    shutdown(descriptor, SHUT_RDWR);

    close(descriptor);

    return (GetUint64Value(low2, high2) - GetUint64Value(low1, high1)) / 1000UL;
}

static uint64_t MeasureTcpBandwidth(int * arguments)
{
    int descriptor;
    struct sockaddr_in server;
    //char * txMessage = "Hello World";
    char message[1400];
    unsigned int low1, low2, high1, high2;
    int i;

    memset(message, 0x5A, 1400);
    descriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_family = AF_INET;
    server.sin_port = htons(7);
    server.sin_addr.s_addr = arguments[0];

    connect(descriptor, (const struct sockaddr *)&server, sizeof(server));
   
    GetRdtscpValue(&low1, &high1);

    for (i = 0; i < 1000; i++)
    {
        send(descriptor, message, strlen(message), 0);

        recv(descriptor, message, strlen(message), 0); 
    }

    GetRdtscpValue(&low2, &high2);

    shutdown(descriptor, SHUT_RDWR);

    close(descriptor);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);

}

static uint64_t MeasureTcpSetup(int * arguments)
{
    int descriptor;
    struct sockaddr_in server;
    char * txMessage = "Hello World";
    char rxMessage[100];
    unsigned int low1, low2, high1, high2;
    
    GetRdtscpValue(&low1, &high1);

    descriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_family = AF_INET;
    server.sin_port = htons(7);
    server.sin_addr.s_addr = arguments[0];

    connect(descriptor, (const struct sockaddr *)&server, sizeof(server));
   
    GetRdtscpValue(&low2, &high2);

    send(descriptor, txMessage, strlen(txMessage), 0);

    recv(descriptor, rxMessage, strlen(txMessage), 0); 

    shutdown(descriptor, SHUT_RDWR);

    close(descriptor);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureTcpTeardown(int * arguments)
{
    int descriptor;
    struct sockaddr_in server;
    char * txMessage = "Hello World";
    char rxMessage[100];
    unsigned int low1, low2, high1, high2;

    descriptor = socket(AF_INET, SOCK_STREAM, 0);
    
    server.sin_family = AF_INET;
    server.sin_port = htons(7);
    server.sin_addr.s_addr = arguments[0];

    connect(descriptor, (const struct sockaddr *)&server, sizeof(server));

    send(descriptor, txMessage, strlen(txMessage), 0);

    recv(descriptor, rxMessage, strlen(txMessage), 0); 

    GetRdtscpValue(&low1, &high1);

    shutdown(descriptor, SHUT_RDWR);

    close(descriptor);
    
    GetRdtscpValue(&low2, &high2);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureFileCache(int * arguments)
{
    int filesize = arguments[0];
    unsigned int low1, low2, high1, high2;
    FILE * stream;
    char * buffer = calloc(ONE_MB * filesize, sizeof(char));
    int i = 0;

    stream = fopen("bigfile", "r");
    
    GetRdtscpValue(&low1, &high1);

    //for (i = 0; i < filesize; i++)
    //{
    //    fread(buffer, sizeof(char), ONE_MB, stream);
    //}

    fread(buffer, sizeof(char), ONE_MB * filesize, stream);

    GetRdtscpValue(&low2, &high2);

    fclose(stream);

    free(buffer);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureFileRead(int * arguments)
{
    int filesize = arguments[0];
    int isLocal = arguments[1];
    unsigned int low1, low2, high1, high2;
    int descriptor;
    char * buffer = calloc(ONE_MB * filesize, sizeof(char));
    int i = 0;
    char * filename = (isLocal) ? "bigfile" : "/illumina/scratch/BaconSoftware/Analysis_Apps/Latest/Images/fastq_Wrap-ChgSet-210888_Isis-2.4.56.img";

    descriptor = open(filename, O_RDONLY);
    
    GetRdtscpValue(&low1, &high1);

    printf("Filesize %d %d\n", ONE_KB * filesize, filesize);
    for (i = 0; i < ONE_KB * filesize; i++)
    {
        pread(descriptor, buffer, ONE_KB, i * ONE_KB);
    }

    //read(descriptor, buffer, ONE_KB * filesize);

    GetRdtscpValue(&low2, &high2);

    close(descriptor);

    free(buffer);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static uint64_t MeasureFileRandomRead(int * arguments)
{
    int filesize = arguments[0];
    int isLocal = arguments[1];
    unsigned int low1, low2, high1, high2;
    int descriptor;
    char * buffer = calloc(ONE_KB, sizeof(char));
    int i = 0; 
    int kbFilesize = filesize * ONE_KB;
    char * filename = (isLocal) ? "bigfile" : "/illumina/scratch/BaconSoftware/Analysis_Apps/Latest/Images/fastq_Wrap-ChgSet-210888_Isis-2.4.56.img";


    descriptor = open(filename, O_RDONLY);
    
    GetRdtscpValue(&low1, &high1);

    for (i = 0; i < kbFilesize; i++)
    {
        pread(descriptor, buffer, ONE_KB, GetPseudoRand() % kbFilesize);
    }

    GetRdtscpValue(&low2, &high2);

    close(descriptor);

    free(buffer);

    return GetUint64Value(low2, high2) - GetUint64Value(low1, high1);
}

static void * ThreadReadBlock(void * arguments)
{
    int number = ((int *)arguments)[0];
    char * buffer = calloc(ONE_MB, sizeof(char));
    char filename[3];
    int descriptor;
    unsigned int low, high;
    uint64_t * value = calloc(1, sizeof(uint64_t));

    sprintf(filename, "%d.bin", number);
    descriptor = open(filename, O_RDONLY);

    pthread_barrier_wait(_barrier);

    GetRdtscpValue(&low, &high);

    read(descriptor, buffer, ONE_MB);

    close(descriptor);

    free(buffer);

    *value = GetUint64Value(low, high);

    pthread_exit(value);
}

static uint64_t MeasureContention(int * arguments)
{
    int threadCount = arguments[0];
    pthread_t threads[40];
    int threadArgs[40];
    int i;
    unsigned int low, high;
    uint64_t minTime = UINT64_MAX;

    _barrier = calloc(1, sizeof(pthread_barrier_t));

    pthread_barrier_init(_barrier, NULL, threadCount);

    for (i = 0; i < threadCount; i++)
    {
        threadArgs[i] = i + 1;
        pthread_create(&threads[i], NULL, ThreadReadBlock, &threadArgs[i]);
    }

    for (i = 0; i < threadCount; i++)
    {   
        uint64_t * time;

        pthread_join(threads[i], (void**)&time);

        if (time[0] < minTime)
        {
            minTime = time[0];
        }
    }

    GetRdtscpValue(&low, &high);

    pthread_barrier_destroy(_barrier);
    free(_barrier);

    return GetUint64Value(low, high) - minTime;
}

