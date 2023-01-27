#ifndef SCHEDULING_SIMULATOR_H
#define SCHEDULING_SIMULATOR_H

#include <stdio.h>
#include <ucontext.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "task.h"

#define STACK_SIZE 16384
enum TASK_STATE {
    TASK_RUNNING,
    TASK_READY,
    TASK_WAITING,
    TASK_TERMINATED
};
typedef struct tinfo {
    int pid;
    int func_index;
    char p;
    char tq;
    int quetime;
    enum TASK_STATE state;
    char stack[STACK_SIZE];
    ucontext_t uctx;
} task_t;

typedef struct waitq {
    int pid;
    int msec;
    int wake;
    int flag;
} waitq_t;

typedef struct func {
    char *name;
    void (*func_ptr)(void);
} func_t;

void hw_suspend(int msec_10);
void hw_wakeup_pid(int pid);
int hw_wakeup_taskname(char *task_name);
int hw_task_create(char *task_name);

#endif
