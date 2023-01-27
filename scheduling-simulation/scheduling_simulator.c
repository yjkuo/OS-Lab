#include "scheduling_simulator.h"

int lowsize=0,highsize=0,pidnum=1,waitindex=-1;
int longtask;
int runindex,lowrunindex,in_high,timer_is_set;/*running task q index*/
int finished,lfinished;/*task terminated index */
int high_q_is_empty=1;
struct itimerval value;
task_t running;
task_t *highq;
task_t *lowq;
waitq_t *waitq;
ucontext_t uctx_main,uctx_sche;

const char *statename[]= {
    "TASK_RUNNING",
    "TASK_READY",
    "TASK_WAITING",
    "TASK_TERMINATED"
};

func_t function[]= {
    {"",NULL},
    {"task1",task1},
    {"task2",task2},
    {"task3",task3},
    {"task4",task4},
    {"task5",task5},
    {"task6",task6}
};
void rmv(task_t q[],int index,int size);
void add(task_t q[],int size,int func,char tq,char prio);
void handler(int sig);
int check_func(char *name);
void terminate(void);
void scheduler(void);

void hw_suspend(int msec_10)
{
    int tmp=++waitindex;
    if(in_high) {
        highq[runindex].state = TASK_WAITING;
        waitq[waitindex].pid = highq[runindex].pid;
    } else {
        lowq[lowrunindex].state = TASK_WAITING;
        waitq[waitindex].pid = lowq[lowrunindex].pid;
    }
    waitq[waitindex].msec = msec_10+1;
    waitq[waitindex].flag = 1;
    scheduler();
    while(!waitq[tmp].wake);
    return;
}

void hw_wakeup_pid(int pid)
{
    for(int i=0; i<highsize; ++i) {
        if(highq[i].pid == pid) {
            highq[i].state = TASK_READY;
            return;
        }
    }
    for(int i=0; i<lowsize; ++i) {
        if(lowq[i].pid == pid) {
            lowq[i].state = TASK_READY;
            return;
        }
    }
}

int hw_wakeup_taskname(char task_name[])
{
    int waked_cnt=0;
    for(int i=0; i<highsize; ++i) {
        if(strcmp(function[highq[i].func_index].name,task_name)==0 && highq[i].state == TASK_WAITING) {
            highq[i].state = TASK_READY;
            waked_cnt++;
        }
    }
    for(int i=0; i<lowsize; ++i) {
        if(strcmp(function[lowq[i].func_index].name,task_name)==0 && lowq[i].state == TASK_WAITING) {
            lowq[i].state = TASK_READY;
            waked_cnt++;
        }
    }
    return waked_cnt;
}

int hw_task_create(char *task_name)
{
    int func;
    if(!(func=check_func(task_name)))
        return -1;
    add(lowq,lowsize,func,'S','L');
    lowsize++;
    return pidnum++; // the pid of created task name
}

int main()
{
    /* shell mode */
    char cmd[10];
    char tmp[10];
    char temptq;
    char stk[STACK_SIZE];
    int fun,is_set,pid,started=0;
    waitq=(waitq_t *)malloc(30*sizeof(waitq_t));
    highq=(task_t *)malloc(20*sizeof(task_t));
    lowq=(task_t *)malloc(50*sizeof(task_t));

    /* Initialize */

    /* Initialize queue quetime */
    for(int i=0; i<10; ++i) {
        highq[i].quetime = 0;
        lowq[i].quetime = 0;
    }
    for(int i=0; i<30; ++i) {
        waitq[i].flag=0;
    }
    /* Initialize timer */
    signal(SIGALRM,handler);
    signal(SIGTSTP,handler);

    getcontext(&uctx_sche);//for terminated
    uctx_sche.uc_stack.ss_sp = stk;
    uctx_sche.uc_stack.ss_size = STACK_SIZE;
    makecontext(&uctx_sche,terminate,0);
    /* shell mode */
    while(1) {
        getcontext(&uctx_main);
        printf("$");
        scanf("%s",cmd);
        if(strcmp(cmd,"add")==0) {
            scanf("%s",tmp);
            if(!(fun=check_func(tmp))) {
                printf("No Function Named %s",tmp);
                return -1;
            }
            temptq='S';
            is_set =0;
            scanf("%[^\n]",tmp);
            char *tok = strtok(tmp," ");
            while(tok != NULL && tok[0] == '-') {
                switch(tok[1]) {
                case 't':
                    tok = strtok(NULL," ");
                    if(strcmp(tok,"L")==0) {
                        temptq = 'L';
                    }
                    break;
                case 'p':
                    tok = strtok(NULL," ");
                    if(strcmp(tok,"H")==0) {

                        add(highq,highsize,fun,temptq,'H');
                        high_q_is_empty=0;
                        highsize++;
                        is_set=1;
                    }
                    break;
                }
                tok = strtok(NULL," ");
            }
            if(!is_set) {
                add(lowq,lowsize,fun,temptq,'L');
                lowsize++;
            }
            pidnum++;
        } else if(strcmp(cmd,"remove")==0) {
            scanf("%d",&pid);
            for(int i=0; i<highsize; ++i) {
                if(highq[i].pid == pid) {
                    rmv(highq,i,highsize);
                    highsize--;
                    break;
                }
            }
            for(int i=0; i<lowsize; ++i) {
                if(lowq[i].pid == pid) {
                    rmv(lowq,i,lowsize);
                    lowsize--;
                    break;
                }
            }
        } else if(strcmp(cmd,"ps")==0) {
            if(!started) {
                puts("simulating...");
                started=1;
                if(!timer_is_set) {
                    value.it_value.tv_sec=0;
                    value.it_value.tv_usec=10000;
                    value.it_interval.tv_sec=0;
                    value.it_interval.tv_usec=10000;
                }
                setitimer(ITIMER_REAL,&value,NULL);
                if(!high_q_is_empty) {
                    highq[0].state = TASK_RUNNING;
                    setcontext(&highq[0].uctx);
                } else {
                    lowq[0].state = TASK_RUNNING;
                    setcontext(&lowq[0].uctx);
                }
            } else {
                for(int i=0; i<highsize; ++i) {
                    printf("%d %s %s %d %c %c\n",highq[i].pid,function[highq[i].func_index].name,
                           statename[highq[i].state],highq[i].quetime,highq[i].p,highq[i].tq);
                }
                for(int i=0; i<lowsize; ++i) {
                    printf("%d %s %s %d %c %c\n",lowq[i].pid,function[lowq[i].func_index].name,
                           statename[lowq[i].state],lowq[i].quetime,lowq[i].p,lowq[i].tq);
                }
            }
        } else if(strcmp(cmd,"start")==0) {
            if(highsize !=0||lowsize!=0) {
                puts("simulating...");
                started=1;
                if(!timer_is_set) {
                    value.it_value.tv_sec=0;
                    value.it_value.tv_usec=10000;
                    value.it_interval.tv_sec=0;
                    value.it_interval.tv_usec=10000;
                }
                setitimer(ITIMER_REAL,&value,NULL);
                if(!high_q_is_empty) {
                    if(highq[runindex].state == TASK_READY)
                        highq[runindex].state = TASK_RUNNING;
                    swapcontext(&uctx_main,&highq[runindex].uctx);
                } else {
                    if(lowq[lowrunindex].state == TASK_READY)
                        lowq[lowrunindex].state = TASK_RUNNING;
                    swapcontext(&uctx_main,&lowq[lowrunindex].uctx);
                }
            }
        }

    }
    return 0;
}

void rmv(task_t q[],int index,int size)
{
    for(int i=index; i<size; ++i) {
        q[i]=q[i+1];
    }
}
int check_func(char *name)
{
    for(int i=1; i<=6; ++i) {
        if(strcmp(name,function[i].name)==0)
            return i;
    }
    return 0;
}

void handler(int sig)
{
    struct itimerval tvalue;
    int i;
    switch(sig) {
    case SIGALRM:
        for(i=0; i<=waitindex; ++i) {
            if(waitq[i].flag)
                waitq[i].msec--;
            if(waitq[i].msec == 1) {
                hw_wakeup_pid(waitq[i].pid);
                waitq[i].msec=0;
                waitq[i].wake=1;
                waitq[i].flag=0;
            }
        }
        for(i=0; i<highsize; ++i) {
            if(highq[i].state == TASK_READY)
                highq[i].quetime += 10;
        }
        for(i=0; i<lowsize; ++i) {
            if(lowq[i].state == TASK_READY)
                lowq[i].quetime += 10;
        }
        if(running.tq == 'L')longtask++;
        else scheduler();
        if(longtask==2) {
            longtask=0;
            scheduler();
        }
        break;
    case SIGTSTP:
        tvalue.it_value.tv_sec=0;
        tvalue.it_value.tv_usec=0;
        setitimer(ITIMER_REAL,&tvalue,&value);
        timer_is_set=1;
        printf("\n");
        if(in_high)swapcontext(&highq[runindex].uctx,&uctx_main);
        else swapcontext(&lowq[lowrunindex].uctx,&uctx_main);
        break;

    }

}

void add(task_t q[],int size,int func,char tq,char prio)
{
    q[size].func_index=func;
    q[size].state=TASK_READY;
    q[size].pid = pidnum;
    q[size].tq = tq;
    q[size].p = prio;
    getcontext(&q[size].uctx);
    q[size].uctx.uc_stack.ss_sp = q[size].stack;
    q[size].uctx.uc_stack.ss_size = STACK_SIZE;
    q[size].uctx.uc_link = &uctx_sche;
    makecontext(&q[size].uctx,function[q[size].func_index].func_ptr,0);
}
void terminate(void)
{
    struct itimerval tvalue;
    if(in_high) {
        highq[runindex].state=TASK_TERMINATED;
        finished++;
        if(finished == highsize) {
            if(lfinished == lowsize) { /*stop*/
                tvalue.it_value.tv_sec=0;
                tvalue.it_value.tv_usec=0;
                tvalue.it_interval.tv_sec=0;
                tvalue.it_interval.tv_usec=0;
                setitimer(ITIMER_REAL,&tvalue,&value);
                runindex+=1;
                if(lowrunindex)lowrunindex+=1;
                setcontext(&uctx_main);
            } else {
                high_q_is_empty=1;
                in_high=0;
                if(lowq[lowrunindex].state == TASK_READY)
                    lowq[lowrunindex].state = TASK_RUNNING;
                setcontext(&lowq[lowrunindex].uctx);
            }

        }
    } else {
        lowq[lowrunindex].state=TASK_TERMINATED;
        lfinished++;
        if(lfinished == lowsize && high_q_is_empty) {
            /* simulation stop */
            tvalue.it_value.tv_sec=0;
            tvalue.it_value.tv_usec=0;
            tvalue.it_interval.tv_sec=0;
            tvalue.it_interval.tv_usec=0;
            setitimer(ITIMER_REAL,&tvalue,&value);
            lowrunindex+=1;
            if(runindex)runindex+=1;
            setcontext(&uctx_main);
        }

    }
    scheduler();
}
void scheduler(void)
{

    int n,tmp,flag=0;
    /* reschedule */
    if(!high_q_is_empty) {
        tmp = runindex;
        for(n=0; n<highsize; n++) {
            if(highq[n].state == TASK_READY) {
                flag=1;
                break;
            }
        }
        in_high=1;
        if(flag) {
            if(highq[runindex].state==TASK_RUNNING)
                highq[runindex].state=TASK_READY;
            runindex=n;
            highq[runindex].state = TASK_RUNNING;
            running = highq[runindex];
            swapcontext(&highq[tmp].uctx,&highq[runindex].uctx);
        }
    } else {
        tmp = lowrunindex;
        for(n=0; n<lowsize; n++) {
            if(lowq[n].state == TASK_READY) {
                flag=1;
                break;
            }
        }
        in_high=0;
        if(flag) {
            if(lowq[lowrunindex].state==TASK_RUNNING)
                lowq[lowrunindex].state=TASK_READY;
            lowrunindex=n;
            lowq[lowrunindex].state = TASK_RUNNING;
            running = lowq[lowrunindex];
            swapcontext(&lowq[tmp].uctx,&lowq[lowrunindex].uctx);
        }
    }

}

