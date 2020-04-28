#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "ralloc.h"

int process_count;
int resource_count;
int *available;
int **max_demands;
pthread_mutex_t mutex;
pthread_cond_t *conditions;
int deadlock_handling;
int **needs;
int **allocated;
int **requests;
int *exists;

//boolean array for conditions due to their waiting state
//-1: not waiting
//0: waiting for first case
//1: waiting for second case
int *cond_flag;

// this includes printfs for debugging; you will delete them in final version.

int bankers_algorithm(int work[], int new_allocated[][resource_count], int new_needs[][resource_count])
{
    //boolean finish array
    int finish[process_count];

    for (int i = 0; i < process_count; i++)
    {
        finish[i] = 0;
    }

    int found_process_count = 0;
    int process_found = 0;
    while (found_process_count < process_count)
    {
        process_found = 0;
        for (int i = 0; i < process_count; i++)
        {
            if (finish[i])
            {
                continue;
            }
            int j = 0;
            while (j < resource_count)
            {
                if (new_needs[i][j] > work[j])
                {
                    break;
                }
                j++;
            }

            if (j == resource_count)
            {
                for (int k = 0; k < resource_count; k++)
                {
                    work[k] = work[k] + new_allocated[i][k];
                }
                found_process_count++;
                finish[i] = 1;
                process_found = 1;
            }
        }
        if(!process_found)
        {

            return 0;
        }
    }
    return 1;

}

//type = 1 if demand type is request
//type = -1 if demand type is release
int perform_demand(int pid, int demand[], int type)
{
    for (int i = 0; i < resource_count; i++)
    {
        needs[pid][i] = needs[pid][i] - (type * demand[i]);
        allocated[pid][i] = allocated[pid][i] + (type * demand[i]);
        available[i] = available[i] - (type * demand[i]);
    }
    return 0;
}

int ralloc_init(int p_count, int r_count, int r_exist[], int d_handling)
{
    //printf ("ralloc_init called\n");

    if (p_count > MAX_PROCESSES || r_count > MAX_RESOURCE_TYPES)
    {
        //printf("Invalid process count or resources type count!\n");
        return - 1;
    }
    process_count = p_count;
    resource_count = r_count;
    deadlock_handling = d_handling;
    exists = r_exist;
    max_demands = malloc(p_count * sizeof(int*));
    needs = malloc(p_count * sizeof(int*));
    allocated = malloc(p_count * sizeof(int*));
    requests = malloc(p_count * sizeof(int*));
    for (int i = 0; i < p_count; i++)
    {
        max_demands[i] = malloc(r_count * sizeof(int));
        needs[i] = malloc(r_count * sizeof(int));
        allocated[i] = malloc(r_count * sizeof(int));
        requests[i] = malloc(r_count * sizeof(int));
        for (int j = 0; j < r_count; j++)
        {
            allocated[i][j] = 0;
            requests[i][j] = 0;
            needs[i][j] = 0;
        }
    }

    available = malloc(r_count * sizeof(int));
    for ( int i = 0; i < r_count; i++)
    {
        available[i] = r_exist[i];
    }

    pthread_mutex_init(&mutex, NULL);

    conditions = malloc(p_count * sizeof(pthread_cond_t));
    for (int i = 0; i < p_count; i++)
    {
        pthread_cond_init(&conditions[i], NULL);;
    }

    cond_flag = malloc(p_count * sizeof(int));
    for (int i = 0; i < p_count; i++)
    {
        cond_flag[i] = -1;
    }
    return (0); 
}

int ralloc_maxdemand(int pid, int r_max[]){
    
    //printf ("ralloc_maxdemand called by %d\n", pid);

    for (int i = 0; i < resource_count; i++)
    {
        if (exists[i] < r_max[i])
        {
            return -1;
        }
    }

    for (int i = 0; i < resource_count; i++)
    {
        max_demands[pid][i] = r_max[i];
        needs[pid][i] = r_max[i];
    }

    return (0); 
}

 
int ralloc_request (int pid, int demand[]) {
    pthread_mutex_lock(&mutex);
    //printf("%d requested %d %d %d\n", pid, demand[0], demand[1], demand[2]);
    for (int i = 0; i < resource_count; i++)
    {
        if (needs[pid][i] < demand[i])
        {
            //printf("%d: Invalid request demand!\n", pid);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }


    int found = 0;
    for (int i = 0; i < resource_count; i++)
    {
        if (available[i] < demand[i])
        {
            found = 1;
            break;
        }

        
    }
    if (found == 1)
    {
        cond_flag[pid] = 0;
        
        for (int j = 0; j < resource_count; j++)
        {
            requests[pid][j] = demand[j];
        }
        //printf("%d waiting cond 1\n", pid);
        pthread_cond_wait(&conditions[pid], &mutex);
        //printf("%d passed cond 1\n", pid);
        cond_flag[pid] = -1;
    }

    if (deadlock_handling == DEADLOCK_AVOIDANCE)
    {
        int new_available[resource_count];
        for (int i = 0; i < resource_count; i++)
        {
            new_available[i] = available[i] - demand[i];
        }

        int new_allocated[process_count][resource_count];
        int new_needs[process_count][resource_count];
        for (int i = 0; i < process_count; i++)
        {
            for (int j = 0; j < resource_count; j++)
            {
                if (i == pid)
                {
                    new_allocated[i][j] = allocated[i][j] + demand[j];
                    new_needs[i][j] = needs[i][j] - demand[j];
                }
                else
                {
                    new_allocated[i][j] = allocated[i][j];
                    new_needs[i][j] = needs[i][j];
                }
                               
            }
        }

        int result = bankers_algorithm(new_available, new_allocated, new_needs);
        if (result == 0)
        {
            cond_flag[pid] = 1;
            
            for (int j = 0; j < resource_count; j++)
            {
                requests[pid][j] = demand[j];
            }
            
            //printf("%d waiting cond 2\n", pid);
            pthread_cond_wait(&conditions[pid], &mutex);
            //printf("%d passed cond 2\n", pid);
            cond_flag[pid] = -1;
        }
    }
    
    for (int j = 0; j < resource_count; j++)
    {
        requests[pid][j] = 0;
    }
    //printf("%d allocated %d %d %d\n", pid, demand[0], demand[1], demand[2]);
    perform_demand(pid, demand, 1);
    pthread_mutex_unlock(&mutex);
    return(0); 
}


int ralloc_release (int pid, int demand[]) {
    pthread_mutex_lock(&mutex);
    
    for (int i = 0; i < resource_count; i++)
    {
        if (demand[i] > allocated[pid][i])
        {
            //printf("%d: Invalid release demand!\n", pid);
            pthread_mutex_unlock(&mutex);
            return -1;
        }
    }
    //printf("%d released %d %d %d\n", pid, demand[0], demand[1], demand[2]);
    //RELEASE FIRST!!
    perform_demand(pid, demand, -1);

    int index;
    int fixed = 0;
    for(index = 0; index < process_count; index++)
    {
        if (cond_flag[index] == 0)
        {
            int count;
            for (count = 0; count < resource_count; count++)
            {
                if (available[count] < requests[index][count])
                {
                    break;
                }
            }
            if (count == resource_count) //condition 1 fixed for process with pid = index
            {
                pthread_cond_signal(&conditions[index]);
                pthread_mutex_unlock(&mutex);
                return(0);
                //printf("Signaled thread %d",index);
                fixed = 1;
            }
        }

        if (deadlock_handling == DEADLOCK_AVOIDANCE && cond_flag[index] == 1)
        {
            int new_available[resource_count];
            for (int i = 0; i < resource_count; i++)
            {
                new_available[i] = available[i] - requests[index][i];
            }

            int new_allocated[process_count][resource_count];
            int new_needs[process_count][resource_count];
            for (int i = 0; i < process_count; i++)
            {
                for (int j = 0; j < resource_count; j++)
                {
                    if (i == index)
                    {
                        new_allocated[i][j] = allocated[i][j] + requests[index][j];
                        new_needs[i][j] = needs[i][j] - requests[index][j];
                    }
                    else
                    {
                        new_needs[i][j] = needs[index][j];
                        new_allocated[i][j] = allocated[i][j];
                    }
                                
                }
            }
            int result = bankers_algorithm(new_available, new_allocated, new_needs);
            if (result == 1) //condition 2 fixed for process with pid = index
            {
                pthread_cond_signal(&conditions[index]);
                pthread_mutex_unlock(&mutex);
                return(0);
                //printf("Signaled thread %d",index);
                fixed = 1;
            }
        }
        if (fixed)
        {
            break;
        }
    }

    	pthread_mutex_unlock(&mutex);
    
    return (0); 
}
 
int ralloc_detection(int procarray[]) {
    pthread_mutex_lock(&mutex);
    int available_temp[resource_count];               
    int finish[process_count];     


    for(int i = 0; i < resource_count;i++){
        available_temp[i] = available[i];
    }                   

    for(int i = 0; i  < process_count;i++){
       finish[i] = 1;

       for(int j = 0; j < resource_count;j++){
       		if(requests[i][j] != 0){
       			finish[i] = 0;
       			break;
       		}
       }
    }

    //int added[process_count];

    for(int i = 0; i < process_count;i++){
        	if(finish[i] == 1 ){
	        		for(int k = 0; k < resource_count;k++){
	        			available_temp[k] += allocated[i][k];
	        	}
        	}
    }
     
    //Detection Algorithm Using available_temp,request and allocated
    
    int count = 0;
    while(count < process_count){ //step 2

        int process_found = 0;

        for(int i = 0; i < process_count;i++){

        	if(finish[i] == 1 ){
        		continue;
        	}


        		int j;
        		for (j = 0; j < resource_count;j++){
        			if(requests[i][j] > available_temp[j])
        			{
        				break;
        			}
        		}

        		if(j == resource_count){
        			finish[i] = 1;
        			process_found = 1;
        			count++;

        			for(int k = 0; k < resource_count;k++){
        				available_temp[k] += allocated[i][k]; 
        			}	
        		}
        }

        if(process_found == 0)
        {
            break;
        }

    }

    int deadlock = 0;
    //Step 4
    for(int i = 0; i < process_count;i++){
        if(finish[i] == 0){
        	deadlock++;
        	procarray[i] = 1;
        }
        else{
        	procarray[i] = -1;
        }
    }

    if(deadlock > 0){
    	/*
    	printf("\nAllocated\n");
    	for(int i = 0; i < process_count;i++){
    		for(int j = 0; j < resource_count;j++)
    		{
    			printf("%d ", allocated[i][j]);
    		}
    		printf("\n");
    	}

    	printf("\nRequests\n");
    	for(int i = 0; i < process_count;i++){
    		for(int j = 0; j < resource_count;j++)
    		{
    			printf("%d ", requests[i][j]);
    		}
    		printf("\n");
    	}

    	printf("\nAvailable\n");
    	for(int i = 0; i < resource_count;i++){
    		
    		printf("%d ", available[i]);
    	}

        printf("\n");

         printf("\nDeadlock Processes\n");
    	for(int i = 0; i < process_count;i++){
    		
    		printf("%d ", procarray[i]);
      }*/

        pthread_mutex_unlock(&mutex);
        return deadlock;
    }    

   

    pthread_mutex_unlock(&mutex);
    return (0); 
}

int ralloc_end() {
    /*
	printf("\nAvailable\n");
    	for(int i = 0; i < resource_count;i++){
    		
    		printf("%d ", available[i]);
    	}*/
    for (int i = 0; i < process_count; i++)
    {
        free(max_demands[i]);
        free(allocated[i]);
        free(needs[i]);
        free(requests[i]);
    }

    free(max_demands);
    free(allocated);
    free(needs);
    free(requests);
    free(available);
    free(conditions);
    free(cond_flag);
    return (0); 
}