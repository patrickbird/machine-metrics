#include <stdlib.h>

#include "metric.h"
#include "system.h"

#define SAMPLE_COUNT 50

int main(int argc, char * argv[])
{
    enum MEASUREMENT i;

    InitializeSystem();

    InitializeMetrics((argc <= 1) ? SAMPLE_COUNT : atoi(argv[1]));

    for (i = 0; i < MEASUREMENT_COUNT; i++)
    {
        PerformMeasurement(i);
    }

    FinalizeMetrics();

    return 0;
}
