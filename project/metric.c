#define _GNU_SOURCE

#include <limits.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
    "Fork"
};

static int _dummy;

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

    for (i = PROCEDURE_INITIAL; i <= PROCEDURE_FINAL; i++)
    {
        _metrics[i].Measure = MeasureProcedureCall;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        _metrics[i].Arguments[0] = i;
    }
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

    return ((double)sum / 99.0);
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

