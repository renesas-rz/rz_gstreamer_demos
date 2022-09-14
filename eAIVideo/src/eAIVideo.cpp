#include <gst/gst.h>
#include <glib.h>
#include "eAI_Stream.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include<sys/wait.h>
#include<unistd.h>
#include <pthread.h>

static pthread_t eAIinference_thread;
static pthread_t eAIPostProc_thread;

extern void *R_eAIInference_thread(void *threadid);
extern void *R_eAIPostProc_thread(void *threadid);

static int8_t wait_join(pthread_t *p_join_thread, uint32_t join_time);

int main (int   argc, char *argv[])
{
    int32_t create_thread = -1;
    int8_t ret = 0;

    /* Check input arguments */
    if (argc != 2) {
        g_printerr ("Usage: %s <MP3 filename>\n", argv[0]);
        return -1;
    } 
 
    /*Create Capture Thread*/
    create_thread = pthread_create(&eAIinference_thread, NULL, R_eAIInference_thread, NULL);
    if (0 != create_thread)
    {
        printf("[ERROR] Failed to create Capture Thread.\n");
        goto end_threads;
    }
    /*Create Capture Thread*/
    create_thread = pthread_create(&eAIPostProc_thread, NULL, R_eAIPostProc_thread, NULL);
    if (0 != create_thread)
    {
        printf("[ERROR] Failed to create Capture Thread.\n");
        goto end_threads;
    }
    sleep(2);
    // Start Stream
    eAIStream ( argc, argv);
   
end_threads:
    if(0 == create_thread)
    {
        ret = wait_join(&eAIinference_thread, 20);
        if (0 != ret)
        {
            printf("[ERROR] Failed to exit Capture Thread on time.\n");
        }
    }
    return 0;
}

/*****************************************
* Function Name : wait_join
* Description   : waits for a fixed amount of time for the thread to exit
* Arguments     : p_join_thread = thread that the function waits for to Exit
*                 join_time = the timeout time for the thread for exiting
* Return value  : 0 if successful
*                 not 0 otherwise
******************************************/
static int8_t wait_join(pthread_t *p_join_thread, uint32_t join_time)
{
    int8_t ret_err;
    struct timespec join_timeout;
    ret_err = clock_gettime(CLOCK_REALTIME, &join_timeout);
    if ( 0 == ret_err )
    {
        join_timeout.tv_sec += join_time;
        ret_err = pthread_timedjoin_np(*p_join_thread, NULL, &join_timeout);
    }
    return ret_err;
}
