#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    struct thread_data *td = (struct thread_data *) thread_param;
    if (nanosleep(&td->wait_to_obtain, NULL) != 0) {
        ERROR_LOG("nanosleep: %s", strerror(errno));
        return thread_param;
    }
    if (pthread_mutex_lock(td->mutex) != 0) {
        ERROR_LOG("pthread_mutex_lock: %s", strerror(errno));
        return thread_param;
    }
    if (nanosleep(&td->wait_to_release, NULL) != 0) {
        ERROR_LOG("nanosleep: %s", strerror(errno));
        return thread_param;
    }
    if (pthread_mutex_unlock(td->mutex) != 0) {
        ERROR_LOG("pthread_mutex_unlock: %s", strerror(errno));
        return thread_param;
    }
    td->thread_complete_success = true; /* Mutex not needed here */
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    struct thread_data *td = malloc(sizeof(*td));
    td->mutex = mutex;
    td->wait_to_obtain.tv_sec = 0;
    td->wait_to_obtain.tv_nsec = wait_to_obtain_ms * 1e6;
    td->wait_to_release.tv_sec = 0;
    td->wait_to_release.tv_nsec = wait_to_release_ms * 1e6;
    td->thread_complete_success = false;
    int rc = pthread_create(thread, NULL, threadfunc, td);
    if (rc != 0)
        ERROR_LOG("pthread_create: error %d\n", rc);
    return (rc == 0);
}
