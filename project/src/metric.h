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

    MEM_INITIAL,
    MEM_9,
    MEM_10,
    MEM_11,
    MEM_12,
    MEM_13,
    MEM_14,
    MEM_15,
    MEM_16,
    MEM_17,
    MEM_18,
    MEM_19,
    MEM_20,
    MEM_21,
    MEM_22,
    MEM_23,
    MEM_24,
    MEM_25,
    MEM_26,
    MEM_FINAL,

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
