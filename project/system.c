#include <sched.h>
#include <stdio.h>
#include <sys/resource.h>
#include <sys/time.h>

static void InitializeSubsystem(int (* function)(int *), int * arguments, char * string);
static int SetProcessorAffinity(int * arguments);
static int SetPriority(int * arguments);

extern void IntializeSystem(void)
{
    int argument;

    InitializeSubsystem(&SetProcessorAffinity, NULL, "Processor Affinity");
    
    argument = PRIO_MIN;
    InitializeSubsystem(&SetPriority, &argument, "Priority");
}

static void InitializeSubsystem(int (* function)(int *), int * arguments, char * string)
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

