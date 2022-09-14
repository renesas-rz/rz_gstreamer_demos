#include <gst/gst.h>
#include <glib.h>
#include "gstutils.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <atomic>
#include <string.h>
#include "yolo/yolo.hpp"

#define DISPLAY_WIDTH       (640)
#define DISPLAY_HEIGHT      (480)
#define DISPLAY_CHANNEL     (4)
#define TMP_BUF_SIZE        (DISPLAY_WIDTH*DISPLAY_HEIGHT*DISPLAY_CHANNEL)

typedef struct _CustomData {
     guint sourceid;        /* To control the GSource */
}CustomData;

static FILE *fptr;

static std::atomic<uint32_t> flag_inference (0);
static std::atomic<uint32_t> flag_preprocess (0);
static std::atomic<uint32_t> flag_postprocess (0);
static std::atomic<uint32_t> flag_postproc (0);

static YoloInference eAI;
static uint8_t* input_image;

static CustomData data;

/*Variable for Performance Measurement*/
static struct timespec start_time;
static struct timespec end_time;

/* The appsink has received a buffer */
static GstFlowReturn new_sample (GstElement *sink, CustomData *data) {

  GstSample *sample;
  GstMapInfo map;
  GstBuffer* buffer;
  uint32_t ftmp;
  static int size = 0;
  static bool caponce = true;
  
  /* Retrieve the buffer */
  g_signal_emit_by_name (sink, "pull-sample", &sample);
  if (sample) {
    /* The only thing we do in this example is print a * to indicate a received buffer */
    // Actual compressed image is stored inside GstSample.
    buffer = gst_sample_get_buffer (sample);

    gst_buffer_map (buffer, &map, GST_MAP_READ);
    
    if ( flag_inference.load() == 0 || caponce ) {
    	timespec_get(&start_time, TIME_UTC);
        // eAI Inference ready 
        // Send buffer to eAI
        if ( eAI.getInputBuf() != NULL ) 
        {
        	memcpy(eAI.getInputBuf(), map.data, map.size);
        }
        timespec_get(&end_time, TIME_UTC);

        flag_inference.store(map.size);
        g_print("+");

    } else {
        // Missed frame
        g_print("*");
    }
    
    if ( caponce )
    {
        /* Do this once for debug  */
        size = map.size;
        g_print("\n%d\n", size);
        print_pad_capabilities (sink, "sink");
        caponce = false;
#if 0
        ftmp = open("cap_iamge.raw", O_RDWR);
        write(ftmp, eAI.getInputBuf(), map.size);
        //write(ftmp, map.data, map.size);
        close(ftmp);
#endif
    }
    
    gst_buffer_unmap (buffer, &map);
    gst_sample_unref (sample);
    
    return GST_FLOW_OK;
  }
  return GST_FLOW_ERROR;
}

/* Configure appsink */
void eAI_AppSinkConfigure ( GstElement*  app_sink) {
    GstCaps * caps = gst_caps_from_string("video/x-raw, width=640, height=480, format=BGR");
    g_object_set (app_sink, "emit-signals", TRUE, "caps", caps, NULL);
    g_signal_connect (app_sink, "new-sample", G_CALLBACK (new_sample), &data);
}
void *r_pre_eAIInference_thread(void *theadid)
{


	while(1) {
		if (flag_postprocess.load() != 0 )
		{

			flag_inference.store(1);
		}

	}
}
void *r_post_eAIInference_thread(void *theadid)
{
	while(1) {
		if (flag_inference.load() != 0 )
		{

		}

	}
}
void *R_eAIInference_thread(void *threadid)
{
    uint32_t sz = 0;
    
    // Initialize Inference
    if ( eAI.init() != 0 )
    {
        printf("ERROR: Failed Initialize eAI Inference \n");
    }
    // Allocated buffer for Image post processing
    input_image = (uint8_t*)malloc(sizeof(uint8_t) * eAI.getInImageSize());
    
    printf("eAI Inference Thread Started\n");
    while (1) {
        sz = flag_inference.load();
        if (sz != 0 )
        {

            // printf("Processing AI Inference %d\n", sz);
            if ( eAI.start() != 0)
                break;
            // Copy Image for Post Processing
            memcpy(input_image, eAI.getInputBuf(), sz);
            
            if ( eAI.wait() != 0 )
                break;
            // printf("\nFinished AI Inference\n");
  
            // GST can now copy frame to buffer
            flag_inference.store(0);
            
            // Print Performanc
            //printf("Total: %d\n", (uint32_t)eAI.timedifference_msec(start_time, end_time ));
            //eAI.printPerformance();

            // Get eAI Inference Output
            // Do this here so AI procesing is not started before results copied
            eAI.get_result();

            flag_postproc.store(1);
            

        }
    }
    printf("eAI Ending Thread\n");
    return NULL;
}

void *R_eAIPostProc_thread(void *threadid)
{
    printf("eAI Post Processing Thread Started\n");
    
    while(1) 
    {
        if ( flag_postproc.load() == 1)
        {

        	eAI.yolo_postproc(eAI.getOutputBuf());
            //eAI.print_result_yolo();
            eAI.print_numResutls();
            flag_postproc.store(0);
            
        }
    }
}

