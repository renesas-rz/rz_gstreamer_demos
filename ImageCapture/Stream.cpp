#include <gst/gst.h>
#include <glib.h>
#include "Stream.hpp"

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}


static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
  GstPad *sinkpad;
  GstElement *queue = (GstElement *) data;

  /* We can now link this pad with the vorbis-decoder sink pad */
  g_print ("Dynamic pad created, linking demuxer/decoder\n");

  sinkpad = gst_element_get_static_pad (queue, "sink");

  gst_pad_link (pad, sinkpad);

  gst_object_unref (sinkpad);
}


/*
    This demo plays MP4 video to the Wayland display. 
    It mimics the commndline 
    
    Original resolution
    gst-launch-1.0 filesrc location=./Road.mp4 ! qtdemux ! queue ! h264parse ! queue ! omxh264dec ! queue ! vspmfilter dmabuf-use=true ! waylandsink
    VGA Resolution
    gst-launch-1.0 filesrc location=./Road.mp4 ! qtdemux ! queue ! h264parse ! queue ! omxh264dec ! queue ! vspmfilter dmabuf-use=true ! video/x-raw, width=640, height=480 ! waylandsink
    
    
    The file source name is pasted in as a argument. 
*/

int
eAIStream ( int argc, char *argv[])
{
    GMainLoop *loop;

    GstElement *pipeline, *source, *demuxer, *queue1, *queue2, *queue3, 
	    *parser, *decoder, *sink, *appsink, *vspmfilter;
    GstBus *bus;
    guint bus_watch_id;
    
    GstCaps *filtercaps;
    gint width, height;
    GstPad *pad;
    gchar *capsstr;
    
    GstElementFactory *source_factory, *sink_factory;
    
    /* Initialisation */
    gst_init (&argc, &argv);
 

    loop = g_main_loop_new (NULL, FALSE);


    /* Create gstreamer elements */
    pipeline 	= gst_pipeline_new ("video-player");
    source   	= gst_element_factory_make ("filesrc",       	"source");
    demuxer  	= gst_element_factory_make ("qtdemux",       	"demuxer");
    queue1  	= gst_element_factory_make ("queue",        	"queue1");
    parser	    = gst_element_factory_make ("h264parse",     	"parser");
    queue2  	= gst_element_factory_make ("queue",        	"queue2");
    decoder	    = gst_element_factory_make ("omxh264dec",       "decoder");
    queue3  	= gst_element_factory_make ("queue",        	"queue3");
    sink	    = gst_element_factory_make ("waylandsink", 		"video_sink");
    appsink	    = gst_element_factory_make ("appsink", 		    "app_sink");
    vspmfilter 	= gst_element_factory_make ("vspmfilter",      	"rfilter");

    



    if (!pipeline || !source || !demuxer || !queue1 || !queue2 || !queue3 || !parser || !decoder || !appsink) {
        g_printerr ("One element could not be created. Exiting.\n");
        return -1;
    }

    /* Set up the pipeline */

    /* we set the input filename to the source element */
    g_object_set (G_OBJECT (source), "location", argv[1], NULL);
    g_object_set (G_OBJECT (vspmfilter), "dmabuf-use", true, NULL);
    


    /* we add a message handler */
    bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
    bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
    gst_object_unref (bus);

    
    eAI_AppSinkConfigure( appsink);
    
    /* we add all elements into the pipeline */
    /* file-source | ogg-demuxer | vorbis-decoder | converter | alsa-output */
    gst_bin_add_many (GST_BIN (pipeline),
                    source, demuxer, queue1, queue2, queue3, parser, decoder,               
                    vspmfilter, appsink, NULL);

    /* we link the elements together */
    /* file-source -> ogg-demuxer ~> vorbis-decoder -> converter -> alsa-output */
    gst_element_link (source, demuxer);
    gst_element_link_many ( queue1, parser, queue2, decoder, queue3, 
    vspmfilter, appsink, NULL);

    g_signal_connect (demuxer, "pad-added", G_CALLBACK (on_pad_added), queue1);


    /* note that the demuxer will be linked to the decoder dynamically.
     The reason is that Ogg may contain various streams (for example
     audio and video). The source pad(s) will be created at run time,
     by the demuxer when it detects the amount and nature of streams.
     Therefore we connect a callback function which will be executed
     when the "pad-added" is emitted.*/


    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL, "pipeline");

    /* Set the pipeline to "playing" state*/
    g_print ("Now playing: %s\n", argv[1]);
    gst_element_set_state (pipeline, GST_STATE_PLAYING);


    /* Iterate */
    g_print ("Running...\n");
    g_main_loop_run (loop);


    /* Out of the main loop, clean up nicely */
    g_print ("Returned, stopping playback\n");
    gst_element_set_state (pipeline, GST_STATE_NULL);

    g_print ("Deleting pipeline\n");
    gst_object_unref (GST_OBJECT (pipeline));
    g_source_remove (bus_watch_id);
    g_main_loop_unref (loop);

    return 0;
}
