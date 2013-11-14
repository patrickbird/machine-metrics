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

    MEM_11_S1,
    MEM_12_S1,
    MEM_13_S1,
    MEM_14_S1,
    MEM_15_S1,
    MEM_16_S1,
    MEM_17_S1,
    MEM_18_S1,
    MEM_19_S1,
    MEM_20_S1,
    MEM_21_S1,
    MEM_22_S1,
    MEM_23_S1,
    MEM_24_S1,
    
    MEM_10_S2,
    MEM_11_S2,
    MEM_12_S2,
    MEM_13_S2,
    MEM_14_S2,
    MEM_15_S2,
    MEM_16_S2,
    MEM_17_S2,
    MEM_18_S2,
    MEM_19_S2,
    MEM_20_S2,
    MEM_21_S2,
    MEM_22_S2,
    MEM_23_S2,
    MEM_24_S2,
    
    MEM_10_S3,
    MEM_11_S3,
    MEM_12_S3,
    MEM_13_S3,
    MEM_14_S3,
    MEM_15_S3,
    MEM_16_S3,
    MEM_17_S3,
    MEM_18_S3,
    MEM_19_S3,
    MEM_20_S3,
    MEM_21_S3,
    MEM_22_S3,
    MEM_23_S3,
    MEM_24_S3,
    
    MEM_10_S4,
    MEM_11_S4,
    MEM_12_S4,
    MEM_13_S4,
    MEM_14_S4,
    MEM_15_S4,
    MEM_16_S4,
    MEM_17_S4,
    MEM_18_S4,
    MEM_19_S4,
    MEM_20_S4,
    MEM_21_S4,
    MEM_22_S4,
    MEM_23_S4,
    MEM_24_S4,
    
    MEM_10_S5,
    MEM_11_S5,
    MEM_12_S5,
    MEM_13_S5,
    MEM_14_S5,
    MEM_15_S5,
    MEM_16_S5,
    MEM_17_S5,
    MEM_18_S5,
    MEM_19_S5,
    MEM_20_S5,
    MEM_21_S5,
    MEM_22_S5,
    MEM_23_S5,
   
    
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
