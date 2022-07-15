#include <gst/gst.h>
#include <glib.h>
#include "Stream.hpp"
#include <stdio.h>
#include <stdint.h>

static pthread_t imgCapture_thread;
static int8_t wait_join(pthread_t *p_join_thread, uint32_t join_time);

extern void *R_ImageCapture_thread(void *threadid);

int main (int   argc, char *argv[])
{
    int32_t create_imgCapture_thread = -1;
    int8_t ret = 0;

    /* Check input arguments */
    if (argc != 2) {
        g_printerr ("Usage: %s <MP3 filename>\n", argv[0]);
        return -1;
    } 
 
    /*Create Capture Thread*/
    create_imgCapture_thread = pthread_create(&imgCapture_thread, NULL, R_ImageCapture_thread, NULL);
    if (0 != create_imgCapture_thread)
    {
        printf("[ERROR] Failed to create Capture Thread.\n");
        goto end_threads;
    }

    // Start Stream
    eAIStream ( argc, argv);
   
end_threads:
    if(0 == create_imgCapture_thread)
    {
        ret = wait_join(&imgCapture_thread, 20);
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
