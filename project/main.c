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
    RDTSCP = 0,
    LOOP,
    PROCEDURE_0,
    PROCEDURE_1,
    PROCEDURE_2,
    PROCEDURE_3,
    PROCEDURE_4,
    PROCEDURE_5,
    PROCEDURE_6,
    PROCEDURE_7,
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
    int (* Measure)(int *);
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
    "Procedure (Seven Arguments)"
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

static inline void GetRdtscpValue(unsigned int * low, unsigned int * high)
{
    uint32_t temp;
    __asm__ volatile("rdtscp" : "=a" (*low), "=d" (*high), "=c"(temp): : );
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

static inline uint64_t GetUint64Value(unsigned int low, unsigned int high)
{
    return (uint64_t)low | ((uint64_t)high << 32);
}

static int MeasureRdtscp(int * arguments)
{
    unsigned int rdtsc1Low, rdtsc1High;
    unsigned int rdtsc2Low, rdtsc2High;

    GetRdtscpValue(&rdtsc1Low, &rdtsc1High);
    GetRdtscpValue(&rdtsc2Low, &rdtsc2High);

    return (double)(GetUint64Value(rdtsc2Low, rdtsc2High) - GetUint64Value(rdtsc1Low, rdtsc1High));
}

static int MeasureLoop(int * arguments)
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

    return GetUint64Value(low, high);
}

static uint64_t MeasureSixArguments(int one, int two, int three, int four, int five, int six)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureFiveArguments(int one, int two, int three, int four, int five)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureFourArguments(int one, int two, int three, int four)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureThreeArguments(int one, int two, int three)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureTwoArguments(int one, int two)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureOneArgument(int one)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static uint64_t MeasureZeroArguments(void)
{
    unsigned int low, high;

    GetRdtscpValue(&low, &high);

    return GetUint64Value(low, high);
}

static int MeasureProcedureCall(int * arguments)
{
    int argumentCount = arguments[0];
    int low, high;
    int ticks;

    switch (argumentCount)
    {
        case 0:
            GetRdtscpValue(&low, &high);
            ticks = MeasureZeroArguments();
            break;

        case 1:
            GetRdtscpValue(&low, &high);
            ticks = MeasureOneArgument();
            break;

        case 2:
            GetRdtscpValue(&low, &high);
            ticks = MeasureTwoArguments();
            break;

        case 3:
            GetRdtscpValue(&low, &high);
            ticks = MeasureThreeArguments();
            break;

        case 4:
            GetRdtscpValue(&low, &high);
            ticks = MeasureFourArguments();
            break;

        case 5:
            GetRdtscpValue(&low, &high);
            ticks = MeasureFiveArguments();
            break;

        case 6:
            GetRdtscpValue(&low, &high);
            ticks = MeasureSixArguments();
            break;

        case 7:
            GetRdtscpValue(&low, &high);
            ticks = MeasureSevenArguments();
            break;
    }

    return (int)(ticks - GetUint64Value(low, high));
}

static void InitializeMetrics(int sampleCount)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        _metrics[i].Max = INT_MIN;
        _metrics[i].Min = INT_MAX;
        _metrics[i].Sum = 0;
        _metrics[i].SampleCount = sampleCount;
        _metrics[i].Samples = calloc(_metrics[i].SampleCount, sizeof(double));
        _metrics[i].Name = MetricNames[i];
    }

    _metrics[RDTSCP].Measure = MeasureRdtscp;
    _metrics[RDTSCP].Arguments = NULL;

    _metrics[LOOP].Measure = MeasureLoop;
    _metrics[LOOP].Arguments = NULL;

    for (i = PROCEDURE_0; i <= PROCEDURE_7; i++)
    {
        _metrics[i].Measure = MeasureProcedureCall;
        _metrics[i].Arguments = calloc(1, sizeof(int));
        *_metrics[i].Arguments = i;
    }
}

static void FinalizeMetrics(void)
{
    int i;

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        free(_metrics[i].Samples);
    }

    for (i = PROCEDURE_0; i <= PROCEDURE_7; i++)
    {
        free(_metrics[i].Arguments);
    }
}

static void PerformMeasurement(enum MEASUREMENT measurement)
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
