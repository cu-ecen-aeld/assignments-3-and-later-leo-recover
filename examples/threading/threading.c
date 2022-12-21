#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    DEBUG_LOG("Entering threadfunc...");
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    DEBUG_LOG("Waiting for %d ms...", thread_func_args->wait_to_release_ms);
    usleep(thread_func_args->wait_to_obtain_ms*1000);
    
    DEBUG_LOG("Acquiring mutex...");
    int rc = pthread_mutex_lock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("Error in mutex locking");
    } else {
        DEBUG_LOG("Acquired mutex...");
    }
    
    DEBUG_LOG("Waiting for %d ms...", thread_func_args->wait_to_release_ms);
    usleep(thread_func_args->wait_to_release_ms*1000);
    
    DEBUG_LOG("Unlocking mutex...");
    rc = pthread_mutex_unlock(thread_func_args->mutex);
    if (rc != 0) {
        ERROR_LOG("Error in mutex unlocking");
    } else {
        DEBUG_LOG("Unlocked mutex...");
    }

    thread_func_args->thread_complete_success = true;

    DEBUG_LOG("Exiting threadfunc...");
    return thread_param;
}


/**
 * Allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
 * using threadfunc() as entry point.
 *
 * return true if successful.
 *
 * See implementation details in threading.h file comment block
 */
bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *pdata;

    pdata = (struct thread_data*) malloc(sizeof(struct thread_data));
    
    pdata->mutex = mutex;
    pdata->thread_complete_success = false;
    pdata->wait_to_obtain_ms = wait_to_obtain_ms;
    pdata->wait_to_release_ms = wait_to_release_ms;
    
    int res = pthread_create(thread, NULL, threadfunc, pdata);
    if (res == 0)
    {
        DEBUG_LOG("Created thread...");
        return true;
    }
    else
    {
        ERROR_LOG("Thread creation error: %d", res);
        return false;
    }
}

