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

#define SAMPLE_COUNT 50

enum MEASUREMENT
{
    SYSTEM_OVERHEAD = 0,
    MEASUREMENT_COUNT
};

static struct METRIC
{
    int SampleCount;
    int MinIndex;
    int MaxIndex;
    double Min;
    double Max;
    double Mean;
    double StandardDeviation;
    double Sum;
    double * Samples;
    const char * Name;
    int (* Measure)();
} _metrics[MEASUREMENT_COUNT];

static const char * MetricNames[MEASUREMENT_COUNT] =
{
    "System Overhead"
};

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

static void UpdateMetric(enum MEASUREMENT measurement, int index)
{
    double value = _metrics[measurement].Samples[index];

    if (value < _metrics[measurement].Min)
    {
        _metrics[measurement].Min = value;
        _metrics[measurement].MinIndex = index;
    }
    else if (value > _metrics[measurement].Max)
    {
        _metrics[measurement].Max = value;
        _metrics[measurement].MaxIndex = index;
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

static int GetSystemOverhead(void)
{
    uint64_t rdtsc1, rdtsc2;

    rdtsc1 = GetRdtscValue();
    rdtsc2 = GetRdtscValue();

    return (double)(rdtsc2 - rdtsc1);    
}

static void InitializeMetrics(int sampleCount)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        _metrics->Max = INT_MIN;
        _metrics->Min = INT_MAX;
        _metrics->Sum = 0;
        _metrics->SampleCount = sampleCount;
        _metrics->Samples = calloc(_metrics[i].SampleCount, sizeof(double));
        _metrics->Name = MetricNames[i];
    }

    _metrics[SYSTEM_OVERHEAD].Measure = GetSystemOverhead;
}

static void FinalizeMetrics(void)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        free(_metrics->Samples);
    }
}

static void PerformMeasurement(enum MEASUREMENT measurement)
{
    int i;
    double value;

    for (i = 0; i < _metrics[measurement].SampleCount; i++)
    {
        _metrics[measurement].Samples[i] =  _metrics[measurement].Measure();
        UpdateMetric(measurement, i);
    }

    UpdateMeasurementCalculations(measurement);

    printf("--------------------------------------\n");
    printf("%s\n", _metrics[measurement].Name);
    printf("--------------------------------------\n");
    printf("Min: %f at count %d\n", _metrics[measurement].Min, _metrics[measurement].MinIndex);
    printf("Max: %f at count %d\n", _metrics[measurement].Max, _metrics[measurement].MaxIndex);
    printf("Average: %f\n", _metrics[measurement].Mean);
    printf("Standard Deviation: %f\n", _metrics[measurement].StandardDeviation);
    printf("Sample Count: %d\n\n", _metrics[measurement].SampleCount);
}

int main(int argc, char * argv[])
{
    int arguments[1];
    enum MEASUREMENT i = 0;

    Initialize(&SetProcessorAffinity, NULL, "Processor Affinity");
    
    arguments[0] = PRIO_MIN;
    Initialize(&SetPriority, arguments, "Priority");

    printf("Argument %s\n", argv[1]);
    InitializeMetrics((argc <= 1) ? SAMPLE_COUNT : atoi(argv[1]));

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        PerformMeasurement(i);
    }

    FinalizeMetrics();

	return 0;
}
