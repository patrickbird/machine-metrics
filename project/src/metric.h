#ifndef MEASUREMENT_H
#define MEASUREMENT_H

enum MEASUREMENT
{
    RDTSCP = 0,
    LOOP,

    PROCEDURE_INITIAL,
    PROCEDURE_1,
    PROCEDURE_2,
    PROCEDURE_3,
    PROCEDURE_4,
    PROCEDURE_5,
    PROCEDURE_6,
    PROCEDURE_FINAL,

    SYSTEM_CALL,
    FORK,
    PTHREAD,
    FORK_CONTEXT_SWITCH,
    PTHREAD_CONTEXT_SWITCH,

    MAIN_MEMORY,
    L1_CACHE,
    L2_CACHE,
    BACK_TO_BACK,
    RAM_WRITE_BANDWIDTH,
    RAM_READ_BANDWIDTH,
    PAGE_FAULT,

    MEASUREMENT_COUNT
};

extern void InitializeMetrics(int sampleCount);
extern void FinalizeMetrics(void);
extern void PerformMeasurement(enum MEASUREMENT measurement);

#endif
