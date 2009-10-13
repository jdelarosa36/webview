#include <hildon/hildon-banner.h>
#include <hildon/hildon-program.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>
#include "camera.h"

#ifdef __arm__
/* The device by default supports only
 * vl4l2src for camera and xvimagesink
 * for screen */
#define VIDEO_SRC "v4l2src"
#define VIDEO_SINK "xvimagesink"
#else
/* These are for the X86 SDK. Xephyr doesn't
 * support XVideo extension, so the application
 * must use ximagesink. The video source depends
 * on driver of your Video4Linux device so this
 * may have to be changed */
#define VIDEO_SRC "v4lsrc"
#define VIDEO_SINK "ximagesink"
#endif

typedef struct
{
	HildonWindow *window;
	GstElement *pipeline;
	guint buffer_cb_id;
} AppData;
AppData appdata;

static gboolean buffer_probe_callback(GstElement *image_sink, GstBuffer *buffer, GstPad *pad);
static void bus_callback(GstBus *bus, GstMessage *message);
static gboolean initialize_pipeline();
static gboolean create_jpeg(unsigned char *buffer);
gboolean start_camera(HildonWindow *main_window);
void end_camera();
void take_photo();

static gboolean buffer_probe_callback(
		GstElement *image_sink,
		GstBuffer *buffer, GstPad *pad)
{
	GstMessage *message;
	gchar *message_name;

	unsigned char *data_photo = (unsigned char *) GST_BUFFER_DATA(buffer);

	if(!create_jpeg(data_photo))
		message_name = "photo-failed";
	else
		message_name = "photo-taken";

	g_signal_handler_disconnect(G_OBJECT(image_sink),
			appdata.buffer_cb_id);

	message = gst_message_new_application(GST_OBJECT(appdata.pipeline),
			gst_structure_new(message_name, NULL));
	gst_element_post_message(appdata.pipeline, message);

	return TRUE;
}

void take_photo()
{

	GstElement *image_sink;

	image_sink = gst_bin_get_by_name(GST_BIN(appdata.pipeline),
			"image_sink");

	hildon_banner_show_information(GTK_WIDGET(appdata.window),
		NULL, "Taking Photo");

	appdata.buffer_cb_id = g_signal_connect(
			G_OBJECT(image_sink), "handoff",
			G_CALLBACK(buffer_probe_callback), &appdata);
}


static void bus_callback(GstBus *bus, GstMessage *message)
{
	gchar *message_str;
	const gchar *message_name;
	GError *error;


	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR)
	{
		gst_message_parse_error(message, &error, &message_str);
		g_error("GST error: %s\n", message_str);
		g_free(error);
		g_free(message_str);
	}

	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_WARNING)
	{
		gst_message_parse_warning(message, &error, &message_str);
		g_warning("GST warning: %s\n", message_str);
		g_free(error);
		g_free(message_str);
	}

	if(GST_MESSAGE_TYPE(message) == GST_MESSAGE_APPLICATION)
	{
		message_name = gst_structure_get_name(gst_message_get_structure(message));

		if(!strcmp(message_name, "photo-taken"))
		{
			hildon_banner_show_information(
					GTK_WIDGET(appdata.window),
					NULL, "Photo taken");
		}

		if(!strcmp(message_name, "photo-failed"))
		{
			hildon_banner_show_information(
					GTK_WIDGET(appdata.window),
					"gtk-dialog-error",
					 "Saving photo failed");
		}
	}

}

static gboolean initialize_pipeline()
{
	GstElement *pipeline, *camera_src, *image_sink;
	GstElement *image_queue;
	GstElement *csp_filter, *image_filter, *tee;
	GstCaps *caps;
	GstBus *bus;

	gst_init(NULL, NULL);

	pipeline = gst_pipeline_new("test-camera");

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, (GstBusFunc)bus_callback, &appdata);
	gst_object_unref(GST_OBJECT(bus));

	appdata.pipeline = pipeline;

	camera_src = gst_element_factory_make(VIDEO_SRC, "camera_src");
	csp_filter = gst_element_factory_make("ffmpegcolorspace", "csp_filter");
	tee = gst_element_factory_make("tee", "tee");

	image_queue = gst_element_factory_make("queue", "image_queue");
	image_filter = gst_element_factory_make("ffmpegcolorspace", "image_filter");
	image_sink = gst_element_factory_make("fakesink", "image_sink");

	if(!(pipeline && camera_src && csp_filter
		&& image_queue && image_filter && image_sink))
	{
		g_critical("Couldn't create pipeline elements");
		return FALSE;
	}


	g_object_set(G_OBJECT(image_sink),
			"signal-handoffs", TRUE, NULL);

	gst_bin_add_many(GST_BIN(pipeline), camera_src, csp_filter,
			tee, image_queue,
			image_filter, image_sink, NULL);

	caps = gst_caps_new_simple("video/x-raw-rgb",
			"width", G_TYPE_INT, 640,
			"height", G_TYPE_INT, 480,
			NULL);


	if(!gst_element_link_filtered(camera_src, csp_filter, caps))
	{
		return FALSE;
	}
	gst_caps_unref(caps);

	if(!gst_element_link_many(csp_filter, tee, NULL))
	{
		return FALSE;
	}


	caps = gst_caps_new_simple("video/x-raw-rgb",
			"width", G_TYPE_INT, 640,
			"height", G_TYPE_INT, 480,
			"bpp", G_TYPE_INT, 24,
			"depth", G_TYPE_INT, 24,
			"framerate", GST_TYPE_FRACTION, 15, 1,
			NULL);

	if(!gst_element_link_many(tee, image_queue, image_filter, NULL)) return FALSE;
	if(!gst_element_link_filtered(image_filter, image_sink, caps)) return FALSE;

	gst_caps_unref(caps);

	gst_element_set_state(pipeline, GST_STATE_PLAYING);

	return TRUE;
}

void end_camera()
{
	gst_element_set_state(appdata.pipeline, GST_STATE_NULL);
	gst_object_unref(GST_OBJECT(appdata.pipeline));
}

gboolean start_camera(HildonWindow *main_window)
{

	appdata.window = main_window;


	hildon_banner_show_information(
			GTK_WIDGET(appdata.window),
			NULL,
			"Starting camera");

	if(!initialize_pipeline())
	{
		hildon_banner_show_information(
				GTK_WIDGET(appdata.window),
				"gtk-dialog-error",
				"Failed to initialize pipeline");
		return FALSE;
	}

	return TRUE;
}

/* Creates a jpeg file from the buffer's raw image data */
static gboolean create_jpeg(unsigned char *data)
{
	GdkPixbuf *pixbuf = NULL;
	GError *error = NULL;
	guint height, width, bpp;
	const gchar *directory;
	GString *filename;

	width = 640; height = 480; bpp = 24;

	/* Define the save folder */
	directory = SAVE_FOLDER_DEFAULT;
	if(directory == NULL)
	{
		directory = g_get_tmp_dir();
	}


	/* Create an unique file name */
	filename = g_string_new(g_build_filename("/opt/webview/website/Picture.jpg", NULL));



	/* Create a pixbuf object from the data */
	pixbuf = gdk_pixbuf_new_from_data(data,
			GDK_COLORSPACE_RGB, /* RGB-colorspace */
			FALSE, /* No alpha-channel */
			bpp/3, /* Bits per RGB-component */
			width, height, /* Dimensions */
			3*width, /* Number of bytes between lines (ie stride) */
			NULL, NULL); /* Callbacks */

	/* Save the pixbuf content's in to a jpeg file and check for
	 * errors */
	if(!gdk_pixbuf_save(pixbuf, filename->str, "jpeg", &error, NULL))
	{
		g_warning("%s\n", error->message);
		g_error_free(error);
		gdk_pixbuf_unref(pixbuf);
		g_string_free(filename, TRUE);
		return FALSE;
	}

	/* Free allocated resources and return TRUE which means
	 * that the operation was succesful */
	g_string_free(filename, TRUE);
	gdk_pixbuf_unref(pixbuf);
	return TRUE;
}

