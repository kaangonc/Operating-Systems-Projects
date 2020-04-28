#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "pthread.h"
#include "ralloc.h"

int handling_method;          // deadlock handling method
int N; // process count
int M; //resource type count
int K; //number of for loop
int *processes;

void *aprocess (void *p)
{
    int req[M];
    int k;
    int pid;
    
    pid =  *((int *)p);
    
    printf ("this is thread %d\n", pid);
    fflush (stdout);
    
    for (int i = 0; i < M; i++)
    {
        req[i] = 100;
    }
    ralloc_maxdemand(pid, req);
    /*
    req[0] = 2;
    req[1] = 2;
    req[2] = 2;
    ralloc_request(pid, req);
    sleep(1);
    ralloc_request(pid, req);
    sleep(1);
    ralloc_release(pid, req);
    ralloc_release(pid, req); 
    */
    for (k = 0; k < K; ++k) {
        
        for (int i = 0; i < M; i++)
        {
            req[i] = 2;
        }
        ralloc_request (pid, req);
	
        //sleep(2);

        // call request and release as many times as you wish with
        // different parameters
    }
    
    for (k = 0; k < K; ++k) {
        for (int i = 0; i < M; i++)
        {
            req[i] = 2;
        }
	
        //sleep(2);
        ralloc_release (pid, req);
    }
    


    processes[pid] = 1;
    pthread_exit(NULL); 
}


int main(int argc, char **argv)
{
    N = atoi(argv[1]);
    M = atoi(argv[2]);
    K = atoi(argv[3]);
    int exist[M];
    for (int i = 0; i < M; i++)
    {
        exist[i] = 100;
    }
    int dn; // number of deadlocked processes
    int deadlocked[N]; // array indicating deadlocked processes
    int k;
    int i;
    int pids[N];
    pthread_t tids[N];
    processes = malloc(sizeof(int) * N);

    for (int i = 0; i < N; i++)
    {
        processes[i] = 0;
    }
    
    for (k = 0; k < N; ++k)
        deadlocked[k] = -1; // initialize
    
    handling_method = DEADLOCK_AVOIDANCE;
    int l = ralloc_init (N, M, exist, handling_method);
    if (l == -1)
    {
        exit(1);
    }

    printf ("library initialized\n");
    fflush(stdout);
    
    for (i = 0; i < N; ++i) {
        pids[i] = i;
        pthread_create (&(tids[i]), NULL, (void *) &aprocess,
                        (void *)&(pids[i])); 
    }
    
    printf ("threads created = %d\n", N);
    fflush (stdout);
    
    while (1) {
        //sleep (1); // detection period
        if (handling_method == DEADLOCK_DETECTION) {
            dn = ralloc_detection(deadlocked);
            if (dn > 0) {
                printf ("there are %d deadlocked processes\n", dn);
                //exit(1);
            }
            
        }
        int i;
        for (i = 0; i < N; i++)
        {
            if (processes[i] == 0)
            {
                break;
            }
        }

        if (i == N)
        {
            free(processes);
            ralloc_end();
            return 0;
        }
        // write code for:
        // if all treads terminated, call ralloc_end and exit
    }    
}