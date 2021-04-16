/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        10       /* Number of identical tasks                          */

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];
OS_EVENT     *RandomSem;


OS_EVENT     *PrintBox;
INT8U         TaskData[N_TASKS]   = {1, 3, 3, 5};       /* Parameters to pass to each task               */
int           NumTasks            = 2;                  /* #Tasks                                        */
int			  StartPoint		  = -1;
int           TaskIndex[N_TASKS] = {0};
char          msgBuffer1[40] = {0};
char          msgBuffer2[40] = {0};

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task(void *data);                       /* Function prototypes of tasks                  */
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
        void  PrintTask(void *data);
        void  SortByPeriod(void);
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
    //PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

    //RandomSem   = OSSemCreate(1);                          /* Random number semaphore                  */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    char       s[100];
    INT16S     key;

    PrintBox = OSMboxCreate((void *)0);                /* Create mailbox to print required messages*/
    pdata = pdata;                                         /* Prevent compiler warning                 */

    // TaskStartDispInit();                                   /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    // OSStatInit();                                          /* Initialize uC/OS-II's statistics         */
    TaskStartCreateTasks();                                /* Create all the application tasks         */
    OSTaskDel(OS_PRIO_SELF);
}


/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
    int  i;
    int counter = 0;
    OS_TCB    *ptcb;
    SortByPeriod();
    for (i = 0; i < NumTasks; i++) {                           /* Create Specified Tasks inited at the begining         */
        //printf("\nStart creating task NO%d...", i);
        OSTaskCreate(Task, (void *)&TaskIndex[i], &TaskStk[i][TASK_STK_SIZE - 1], i + 1);
    }
    ptcb = OSTCBList;
    OS_ENTER_CRITICAL();
    while(ptcb->OSTCBPrio != OS_IDLE_PRIO){
        ptcb->OSTCBDeadline = TaskIndex[counter * 2 + 1];
        ++counter;
        ptcb = ptcb->OSTCBNext;
    }
    OS_EXIT_CRITICAL();
    // OSTaskCreate(PrintTask, (void *)NULL, &TaskStk[i][TASK_STK_SIZE - 1], 1);
    //printf("\n%d", OSTaskCtr);
    OSTaskDel(OS_PRIO_SELF);
}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/
void  SortByPeriod  (void) {
    int tempP, tempC, i, j;
    int prio, index;
    for (i = 0; i < NumTasks - 1; i++) {
        prio = TaskData[2 * i + 1];
        index = i;

        for (j = i + 1; j < NumTasks; j++) {
            if (TaskData[2 * j + 1] < prio) {
                index = j;
                prio = TaskData[2 * j + 1];
            }
        }

        if (i != index) {
            TaskData[2 * i + 1] ^= TaskData[2 * index + 1];
            TaskData[2 * i] ^= TaskData[2 * index];

            TaskData[2 * index + 1] ^= TaskData[2 * i + 1];
            TaskData[2 * index] ^= TaskData[2 * i];

            TaskData[2 * i + 1] ^= TaskData[2 * index + 1];
            TaskData[2 * i] ^= TaskData[2 * index];
        }
    }

    for (i = 0; i < NumTasks; i++) {
        //printf("\n%d %d", TaskData[ 2 * i], TaskData[ 2 * i + 1]);
        TaskIndex[i] = i;
    }
}

void  Task (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    int    start = OSTimeGet();
    int    end;
    int    toDelay;
    int    index = *((int*)pdata);
    int    deadline;
    OS_ENTER_CRITICAL();
    if (StartPoint > 0) {
    	start = StartPoint;
    } else {
    	StartPoint = start;
    }
    OSTCBCur->OSTCBPeriod = TaskData[2 * index + 1];
    OSTCBCur->OSTCBcompTime = TaskData[2 * index];
    OS_EXIT_CRITICAL()


    while (1) {
        while (OSTCBCur->OSTCBcompTime > 0) {
            // task executing
        	//printf("%d \n", monitor);
        	end = OSTimeGet();
        	if( end >= OSTCBCur->OSTCBPeriod + start){
        		printf("\n%6d Task %d Missed", OSTCBCur->OSTCBPeriod + start, index+1);
        		break;
        	}
        }
        end = OSTimeGet();
        toDelay = OSTCBCur->OSTCBPeriod - (end - start);
        start = start + OSTCBCur->OSTCBPeriod;
        deadline = start + OSTCBCur->OSTCBPeriod;
        OS_ENTER_CRITICAL();
        OSTCBCur->OSTCBcompTime = TaskData[2 * index];
        OSTCBCur->OSTCBDeadline = deadline;
        OS_EXIT_CRITICAL();
        //printf("\n start: %d, end: %d, delay: %d\nprio: %d, period: %d, compTime: %d, deadline: %d\n", start-OSTCBCur->OSTCBPeriod, end, toDelay, OSTCBCur->OSTCBPrio, OSTCBCur->OSTCBPeriod, OSTCBCur->OSTCBcompTime, deadline);
        if (toDelay > 0) {
            OSTimeDly(toDelay);
        }
    }
}


/* not used */
void PrintTask(void *pdata) {
    INT8U   err;
    INT16S  key;
    pdata = pdata;
    while(1) {
        OSMboxPend(PrintBox, 0, &err);
        printf("%s", msgBuffer1);
        OSTimeDly(1);
    }
}
