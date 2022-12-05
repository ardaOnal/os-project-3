#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "linkedList.c"
#include <sys/time.h>
#include <math.h>

const int MINIMUM_GRANULARITY = 10;
const int SCHED_LATENCY = 100;

int ALLP;
int rqLen;
int OUTMODE;

struct Node* runqueue = NULL;
pthread_mutex_t mutex_lock;

int finishedProcessCount = 0;
pthread_cond_t scheduler_cv;
int counter = 0;

struct timeval programStartTime;

struct PCB** finishedProcesses;

char* OUTFILE;
int isOutfile = 0;
FILE* fptr = NULL;


void* schedulerThread( void* arg_ptr){
    pthread_mutex_lock(&mutex_lock);
    while(finishedProcessCount < ALLP){
        pthread_cond_wait(&scheduler_cv, &mutex_lock);
        if(runqueue){
            struct PCB* minVruntime;
            
            getMinCFS(runqueue, &minVruntime);
            if(OUTMODE == 3){
                if(isOutfile == 0)
                    printf("Process %d is selected for CPU\n", minVruntime->pid);
                else if( isOutfile == 1){
                    fprintf(fptr, "Process %d is selected for CPU\n", minVruntime->pid);
                }
            }
            
            pthread_cond_signal(&(minVruntime->cv));
        }       
    }
    pthread_mutex_unlock(&mutex_lock);

    
}

void* processThread( void* arg_ptr){

    struct PCB* pcb = malloc(sizeof(struct PCB));

    pthread_cond_init(&pcb->cv, NULL);
    pcb->pid = ((struct processThreadArgs *) arg_ptr)->pid;
    pcb->priority = ((struct processThreadArgs *) arg_ptr)->priority;
    pcb->processLength = ((struct processThreadArgs *) arg_ptr)->processLength;
    pcb->totalTimeSpent = 0;
    pcb->vruntime = 0;

    pcb->contextSwitch = -1;

    struct timeval threadStart;
    gettimeofday(&threadStart, NULL);
    pcb->arrivalTime = 1000 * ((threadStart.tv_sec - programStartTime.tv_sec) + ((threadStart.tv_usec - programStartTime.tv_usec)/1000000.0));

    pthread_mutex_lock(&mutex_lock);
    if(OUTMODE == 3){
        if(isOutfile == 0)
            printf("Process %d inserted to runqueue\n", pcb->pid);
        else if( isOutfile == 1)
            fprintf(fptr, "Process %d inserted to runqueue\n", pcb->pid);
    }
        
    insert(&runqueue, pcb);
    pthread_cond_signal(&scheduler_cv);
    while(pcb->totalTimeSpent < pcb->processLength){
        pthread_cond_wait(&(pcb->cv), &(mutex_lock));
        

        double weightDenominator = getAllWeights(runqueue);

        int timeslice = (prio_to_weight[pcb->priority + 20]) / weightDenominator * SCHED_LATENCY;

        struct Node* dequeuedNode = dequeueNode(&runqueue, findIndexByPid(runqueue, pcb->pid));
        if(timeslice < MINIMUM_GRANULARITY)
            timeslice = MINIMUM_GRANULARITY;
        if(pcb->processLength - pcb->totalTimeSpent < timeslice){
            timeslice = pcb->processLength - pcb->totalTimeSpent;
        }
        if(OUTMODE == 2){
            struct timeval curTime;
            gettimeofday(&curTime, NULL);
            float myCurrentTimeForNow = 1000 * ((curTime.tv_sec - programStartTime.tv_sec) + 
                                                            ((curTime.tv_usec - programStartTime.tv_usec)/1000000.0));
    
            if(isOutfile == 0)        
                printf("%f %d RUNNING %d\n", myCurrentTimeForNow, pcb->pid, timeslice);
            else if( isOutfile == 1)
                fprintf( fptr, "%f %d RUNNING %d\n", myCurrentTimeForNow, pcb->pid, timeslice);
        }
        if(OUTMODE == 3){
            if(isOutfile == 0)        
                printf("Process %d RUNNNING in CPU\n", pcb->pid);
            else if( isOutfile == 1)
                fprintf( fptr, "Process %d RUNNNING in CPU\n", pcb->pid);
        }
            
        
        usleep(timeslice * 1000);

        if(OUTMODE == 3){
            if(isOutfile == 0)        
                printf("Process %d expired timeslice %d\n", pcb->pid, timeslice);
            else if( isOutfile == 1)
                fprintf( fptr, "Process %d expired timeslice %d\n", pcb->pid, timeslice);
        }
        //increment total time spent
        pcb->totalTimeSpent = pcb->totalTimeSpent + timeslice;
        pcb->contextSwitch = pcb->contextSwitch + 1;

        //vruntime
        pcb->vruntime = pcb->vruntime + (prio_to_weight[20] / prio_to_weight[pcb->priority + 20] * (double)(timeslice));

        
        if( pcb->totalTimeSpent >= pcb->processLength){
            
            finishedProcessCount++;

            struct timeval threadFinish;
            gettimeofday(&threadFinish, NULL);
            pcb->finishTime = 1000 * ((threadFinish.tv_sec - programStartTime.tv_sec) + 
                                                            ((threadFinish.tv_usec - programStartTime.tv_usec)/1000000.0));
            pcb->queueWaitTime = pcb->finishTime - pcb->arrivalTime - pcb->totalTimeSpent;

            finishedProcesses[pcb->pid - 1] = pcb;
            
            pthread_cond_destroy(&(pcb->cv));
            
            pthread_cond_signal(&scheduler_cv);
            break;
        }
        else{
            if(OUTMODE == 3){
                if(isOutfile == 0)        
                    printf("Process %d inserted to runqueue\n", pcb->pid);
                else if( isOutfile == 1)
                    fprintf( fptr, "Process %d inserted to runqueue\n", pcb->pid);
            }
            
            insert(&runqueue, dequeuedNode->pcb);
        }
        if(dequeuedNode){
            free(dequeuedNode);
        }  
        pthread_cond_signal(&scheduler_cv);
    }
    
    
    if(OUTMODE == 3){
        if(isOutfile == 0)        
            printf("Process %d finished\n", pcb->pid);
        else if( isOutfile == 1)
            fprintf( fptr, "Process %d finished\n", pcb->pid);
    }
    pthread_mutex_unlock(&mutex_lock);

    
    
}

void* generatorThread(void* arg_ptr){



    int* processLengths = ((struct generatorArgs *) arg_ptr)->processLengths;
    int* priorityValues = ((struct generatorArgs *) arg_ptr)->priorityValues;
    int plCount = ((struct generatorArgs *) arg_ptr)->plCount;
    int* interarrivalTimes = ((struct generatorArgs *) arg_ptr)->interarrivalTimes;

    pthread_t processThreadIds[plCount];
    for(int i = 0; i < plCount; ){
        struct processThreadArgs processThreadArgs;
        processThreadArgs.pid = i+1;
        processThreadArgs.priority = priorityValues[i];
        processThreadArgs.processLength = processLengths[i];
        processThreadArgs.totalTimeSpent = 0;

        pthread_mutex_lock(&mutex_lock);
        if( runqueueSize < rqLen){
            if(OUTMODE == 3){
                if(isOutfile == 0)        
                    printf("Process %d created\n", processThreadArgs.pid);
                else if( isOutfile == 1)
                    fprintf( fptr, "Process %d created\n", processThreadArgs.pid);
            }
            int ret = pthread_create(&processThreadIds[i], NULL, processThread, (void*) &processThreadArgs);
            
            pthread_mutex_unlock(&mutex_lock);
            if( i < plCount - 1)
                usleep(1000 * interarrivalTimes[i]);
            i++;
            if( ret){
                printf("ERROR creating thread\n");
            }
        }
        else{
            pthread_mutex_unlock(&mutex_lock);
            usleep(1000 * interarrivalTimes[i-1]);
        }   
    }
    pthread_exit(NULL);
    
}

int main(int argc, char* argv[]){ 
    if ( argc > 5) {


        gettimeofday(&programStartTime, NULL);


        pthread_mutex_init(&mutex_lock, NULL);
        pthread_cond_init(&scheduler_cv, NULL);

        // Command Line

        if ( argv[1][0] == 'C') {
            int minPrio = atoi(argv[2]);
            int maxPrio = atoi(argv[3]);
            char* distPL = argv[4];
            int avgPL = atoi(argv[5]); 
            int minPL = atoi(argv[6]);
            int maxPL = atoi(argv[7]);
            char* distIAT = argv[8];
            int avgIAT = atoi(argv[9]); 
            int minIAT = atoi(argv[10]);
            int maxIAT = atoi(argv[11]);
            rqLen = atoi(argv[12]);
            ALLP = atoi(argv[13]);
            OUTMODE = atoi(argv[14]);
            if ( argc == 16){
                OUTFILE = argv[15];
                isOutfile = 1;
                fptr = fopen(OUTFILE, "w");
                if(fptr == NULL)
                    printf("Error opening the file named %s\n", OUTFILE);
            }
            
            /*printf("\nCommand line\n");
            printf("Arguments:\n");
            printf("minPrio %d\n", minPrio);
            printf("maxPrio %d\n", maxPrio);
            printf("distPL %s\n", distPL);
            printf("avgPL %d\n", avgPL);
            printf("minPL %d\n", minPL);
            printf("maxPL %d\n", maxPL);
            printf("distIAT %s\n", distIAT);
            printf("avgIAT %d\n", avgIAT);
            printf("minIAT %d\n", minIAT);
            printf("maxIAT %d\n", maxIAT);
            printf("rqLen %d\n", rqLen);
            printf("ALLP %d\n", ALLP);
            printf("OUTMODE %d\n", OUTMODE);
            if ( argc == 16) printf("OUTFILE %s\n", OUTFILE);*/

            finishedProcesses = malloc(sizeof(struct PCB*) * ALLP);

            int processLengths[ALLP];
            int priorityValues[ALLP];
            int interarrivalTimes[ALLP-1];
            srand(time(NULL));
            //RANDOM PRIO VALUES
            for(int i = 0; i < ALLP; i++){
                priorityValues[i] = (rand() % (maxPrio - minPrio + 1) + minPrio);
            }
            //PROCESS LENGTH
            if(strcmp(distPL, "fixed") == 0){
                for(int i = 0; i < ALLP; i++){
                    processLengths[i] = avgPL;
                }
            }
            else if( strcmp(distPL, "uniform") == 0){
                for(int i = 0; i < ALLP; i++){
                    processLengths[i] = (rand() % (maxPL - minPL + 1) + minPL);
                }
                
            }
            else if( strcmp(distPL, "exponential") == 0){
                double lambda = 1.0/avgPL;
                

                for(int i = 0; i < ALLP; i++){
                    double x = minPL - 2;
                    while( x <= minPL || x >= maxPL){
                        double u = (double)rand() / (double)RAND_MAX;
                        x = ((-1.0) * log(1-u))/lambda;
                    }
                    processLengths[i] = (int) round(x);
                }    
            }

            //IAT
            if(strcmp(distIAT, "fixed") == 0){
                for(int i = 0; i < ALLP-1; i++){
                    interarrivalTimes[i] = avgIAT;
                }
            }
            else if( strcmp(distIAT, "uniform") == 0){
                for(int i = 0; i < ALLP - 1; i++){
                    interarrivalTimes[i] = (rand() % (maxIAT - minIAT + 1) + minIAT);
                }
                
            }
            else if( strcmp(distIAT, "exponential") == 0){
                double lambda = 1.0/avgIAT;
                

                for(int i = 0; i < ALLP - 1; i++){
                    double x = minIAT - 2;
                    while( x <= minIAT || x >= maxIAT){
                        double u = (double)rand() / (double)RAND_MAX;
                        x = ((-1.0) * log(1-u))/lambda;
                    }
                    interarrivalTimes[i] = (int) round(x);
                }    
            }

            
            struct generatorArgs generator_args;
            generator_args.processLengths = processLengths;
            generator_args.interarrivalTimes = interarrivalTimes;
            generator_args.priorityValues = priorityValues;
            generator_args.plCount = ALLP;
            
            pthread_t thr_id[2];
            int ret = pthread_create(&thr_id[0], NULL, schedulerThread, (void*) NULL);

            if( ret){
                printf("ERROR creating scheduler thread\n");
            }
            ret = pthread_create(&thr_id[1], NULL, generatorThread, (void*) &generator_args);

            if( ret){
                printf("ERROR creating generator thread\n");
            }

            

            pthread_join(thr_id[0], NULL);
            pthread_join(thr_id[1], NULL);
            float totalWaitingTime = 0;


            if(isOutfile == 0)
                printf("%5s %15s %15s %5s %8s %15s %15s %10s\n", "pid", "arv" , "dept" , "prio","cpu" ,"waitr" ,"turna","cs");
            else if(isOutfile == 1)
                fprintf(fptr, "%5s %15s %15s %5s %8s %15s %15s %10s\n", "pid", "arv" , "dept" , "prio","cpu" ,"waitr" ,"turna","cs");
            for(int i = 0; i < ALLP; i++){
                struct PCB* curPCB = finishedProcesses[i];
                if(isOutfile == 0)
                    printf("%5d %15f %15f %5d %8d %15f %15f %10d\n",
                                curPCB->pid, curPCB->arrivalTime, curPCB->finishTime, curPCB->priority, curPCB->totalTimeSpent, curPCB->queueWaitTime,
                                curPCB->finishTime - curPCB->arrivalTime, curPCB->contextSwitch);
                else if( isOutfile == 1)
                    fprintf(fptr, "%5d %15f %15f %5d %8d %15f %15f %10d\n",
                                curPCB->pid, curPCB->arrivalTime, curPCB->finishTime, curPCB->priority, curPCB->totalTimeSpent, curPCB->queueWaitTime,
                                curPCB->finishTime - curPCB->arrivalTime, curPCB->contextSwitch);
                totalWaitingTime = totalWaitingTime + curPCB->queueWaitTime;
            }
            if(isOutfile == 0)
                printf("avg waiting time: %f\n", totalWaitingTime / ALLP);
            else if(isOutfile == 1)
                fprintf(fptr, "avg waiting time: %f\n", totalWaitingTime / ALLP);


            for(int i = 0; i < ALLP; i++){
                free(finishedProcesses[i]);
            }
            free(finishedProcesses);
            pthread_mutex_destroy(&mutex_lock);
            pthread_cond_destroy(&scheduler_cv);
            
        } 
        // Input file
        else if (argv[1][0] == 'F' ) {
            //printf("\nInput file\n");
            rqLen = atoi(argv[2]);
            ALLP = atoi(argv[3]);
            OUTMODE = atoi(argv[4]);
            char* INFILE = argv[5];
            
            if ( argc == 7){
                OUTFILE = argv[6];
                isOutfile = 1;
                fptr = fopen(OUTFILE, "w");
                if(fptr == NULL)
                    printf("Error opening the file named %s\n", OUTFILE);
            }

            finishedProcesses = malloc(sizeof(struct PCB*) * ALLP);

            FILE* fp;
            char * line = NULL;
            fp = fopen(INFILE, "r");

            if (fp == NULL)
            {
                printf("Could not open file %s", INFILE);
                return 0;
            }
            
            char type[20]; // PL or IAT
            int value1;
            int value2;
            int lineInt = fscanf(fp,"%s %d %d",type,&value1,&value2);

            // Values as arrays from input file
            int processLengths[ALLP];
            int plCount = 0;
            int priorityValues[ALLP];
            int pvCount = 0;
            int interarrivalTimes[ALLP-1];
            int iaCount = 0;

            while(lineInt != -1)  {
                if ( lineInt == 3) {
                    if ( strcmp(type,"PL") == 0) {
                        processLengths[plCount++] = value1;
                        priorityValues[pvCount++] = value2;
                    }
                } else if (lineInt == 2) {
                    if ( strcmp(type,"IAT") == 0) {
                        interarrivalTimes[iaCount++] = value1;
                    }
                }

                lineInt = fscanf(fp,"%s %d %d",type,&value1,&value2);
            }

            fclose(fp);
            
            struct generatorArgs generator_args;
            generator_args.processLengths = processLengths;
            generator_args.interarrivalTimes = interarrivalTimes;
            generator_args.priorityValues = priorityValues;
            generator_args.plCount = plCount;
            
            pthread_t thr_id[2];
            int ret = pthread_create(&thr_id[0], NULL, schedulerThread, (void*) NULL);

            if( ret){
                printf("ERROR creating schedular thread\n");
            }
            ret = pthread_create(&thr_id[1], NULL, generatorThread, (void*) &generator_args);

            if( ret){
                printf("ERROR creating generator thread\n");
            }

            

            pthread_join(thr_id[0], NULL);
            pthread_join(thr_id[1], NULL);
            float totalWaitingTime = 0;


            if(isOutfile == 0)
                printf("%5s %15s %15s %5s %8s %15s %15s %10s\n", "pid", "arv" , "dept" , "prio","cpu" ,"waitr" ,"turna","cs");
            else if(isOutfile == 1)
                fprintf(fptr, "%5s %15s %15s %5s %8s %15s %15s %10s\n", "pid", "arv" , "dept" , "prio","cpu" ,"waitr" ,"turna","cs");
            for(int i = 0; i < ALLP; i++){
                struct PCB* curPCB = finishedProcesses[i];
                if(isOutfile == 0)
                    printf("%5d %15f %15f %5d %8d %15f %15f %10d\n",
                                curPCB->pid, curPCB->arrivalTime, curPCB->finishTime, curPCB->priority, curPCB->totalTimeSpent, curPCB->queueWaitTime,
                                curPCB->finishTime - curPCB->arrivalTime, curPCB->contextSwitch);
                else if( isOutfile == 1)
                    fprintf(fptr, "%5d %15f %15f %5d %8d %15f %15f %10d\n",
                                curPCB->pid, curPCB->arrivalTime, curPCB->finishTime, curPCB->priority, curPCB->totalTimeSpent, curPCB->queueWaitTime,
                                curPCB->finishTime - curPCB->arrivalTime, curPCB->contextSwitch);
                totalWaitingTime = totalWaitingTime + curPCB->queueWaitTime;
            }
            if(isOutfile == 0)
                printf("avg waiting time: %f\n", totalWaitingTime / ALLP);
            else if(isOutfile == 1)
                fprintf(fptr, "avg waiting time: %f\n", totalWaitingTime / ALLP);

            for(int i = 0; i < ALLP; i++){
                free(finishedProcesses[i]);
            }
            free(finishedProcesses);
            pthread_mutex_destroy(&mutex_lock);
            pthread_cond_destroy(&scheduler_cv);


        }
        if(isOutfile == 1)
            fclose(fptr);
    }


}