#include <gtk/gtk.h>
#include <hildon/hildon-program.h>
#include <gconf/gconf-client.h>
#include <glib/gstdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include "camera.h"


#define STRING_MAX_SIZE 4096

int update_interval = 120;
gchar *base_url =  "http://localhost:";
gchar *server_port = "80";
gboolean timmer_running = FALSE;

GtkWidget *image;
gchar *image_file =  "/opt/webview/website/Picture.jpg";

gchar *app_dir = "/opt/webview/";
gchar *web_server_script = "webview.d";
gchar *web_server_dir = "/opt/webview/website";
gchar *config_dir = "/opt/webview/config";
gchar *web_config = "thttpd.conf";
gchar *web_log_file = "/opt/webview/logs/thttpd_log";

GtkWidget *txtServerPort;
GtkWidget *txtUpdateInterval;
GtkWidget *lblUpdate;

extern void take_photo();
extern gboolean start_camera();
extern void end_camera();

static void start_button_clicked(GtkWidget *window, gpointer data);
static void stop_button_clicked(GtkWidget *window, gpointer data);
static void refresh_button_clicked(GtkWidget *window, gpointer data);

void destroy (GtkWidget *widget, gpointer data);

static void on_btnSave_clicked(GtkWidget *button, gpointer userdata);

static void menu_settings_clicked(GtkMenuItem *menuitem, gpointer data);
static void menu_exit_clicked(GtkMenuItem *menuitem, gpointer data);
static void menu_about_clicked(GtkMenuItem *menuitem, gpointer data);

void set_open_web_button(GtkWidget *web_link_button);
void update_image();
void *count_down_thread();
gboolean start_webserver();
gboolean stop_webserver();
gboolean update_http_config();

const gchar *GC_ROOT =  "/apps/gladetest/";
void load_settings();
void save_settings();

int main(int argc, char *argv[]) {

	GtkBuilder *builder;
	GtkWidget  *window;
	GError     *error = NULL;
	pthread_t thread;

    /* init threads */
    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();

	gtk_init( &argc, &argv );

	/*load main dialog */
	builder = gtk_builder_new();
	if( ! gtk_builder_add_from_file( builder, "src/dialog.xml", &error ) )
	{
		g_warning( "%s", error->message );
		g_free( error );
		return( 1 );
	}
	window = GTK_WIDGET( gtk_builder_get_object( builder, "window1" ) );

	gtk_signal_connect (GTK_OBJECT (window), "destroy", GTK_SIGNAL_FUNC (destroy), NULL);

	GtkWidget  *btnStart = GTK_WIDGET(gtk_builder_get_object (builder, "btnStart"));
    g_signal_connect (G_OBJECT (btnStart), "clicked", G_CALLBACK (start_button_clicked), NULL);

    GtkWidget  *btnStop = GTK_WIDGET(gtk_builder_get_object (builder, "btnStop"));
    g_signal_connect (G_OBJECT(btnStop), "clicked", G_CALLBACK (stop_button_clicked), NULL);

	GtkWidget *btnRefresh = GTK_WIDGET(gtk_builder_get_object (builder, "btnRefresh"));
    g_signal_connect (G_OBJECT (btnRefresh), "clicked", G_CALLBACK (refresh_button_clicked), NULL);

    GtkWidget *lbOpenWebPage = GTK_WIDGET(gtk_builder_get_object (builder, "lbOpenWebPage"));
    set_open_web_button(lbOpenWebPage);

	GtkWidget *mnuSettings = GTK_WIDGET(gtk_builder_get_object (builder, "mnuSettings"));
    g_signal_connect (G_OBJECT (mnuSettings), "activate", GTK_SIGNAL_FUNC (menu_settings_clicked), NULL);

	GtkWidget *mnuExit = GTK_WIDGET(gtk_builder_get_object (builder, "mnuExit"));
	g_signal_connect(G_OBJECT (mnuExit), "activate", GTK_SIGNAL_FUNC (menu_exit_clicked), NULL);

	GtkWidget *mnuAbout = GTK_WIDGET(gtk_builder_get_object (builder, "mnuAbout"));
	g_signal_connect(G_OBJECT (mnuAbout), "activate", GTK_SIGNAL_FUNC (menu_about_clicked), NULL);

	lblUpdate = GTK_WIDGET(gtk_builder_get_object (builder, "lblUpdate"));

	image = GTK_WIDGET(gtk_builder_get_object (builder, "image"));
	update_image();

	load_settings();

	if(! start_camera(window))
	{
		GtkWidget *failDialog = gtk_message_dialog_new(NULL,
	     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
	     		GTK_BUTTONS_OK,
	     		"Unable to start camera\n");
		gtk_dialog_run (GTK_DIALOG (failDialog));
		gtk_widget_destroy (failDialog);
	}

	if(! start_webserver())
	{
		GtkWidget *failDialog = gtk_message_dialog_new(NULL,
	     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
	     		GTK_BUTTONS_OK,
	     		"Unable to start web server.\nCheck server port is not already in use.\n");
		gtk_dialog_run (GTK_DIALOG (failDialog));
		gtk_widget_destroy (failDialog);
	}


	g_object_unref( G_OBJECT( builder ) );
	gtk_widget_show( window );

	pthread_create(&thread, NULL, count_down_thread, NULL);
	gtk_main();

	gdk_threads_leave ();
	return( 0 );
}

void
destroy (GtkWidget *widget, gpointer data)
{
	stop_webserver();
	 end_camera();
     gtk_main_quit ();
}

void
set_open_web_button(GtkWidget *web_link_button)
{
	gchar *web_url = g_strconcat(base_url, server_port, NULL);
	gtk_link_button_set_uri(GTK_LINK_BUTTON(web_link_button), web_url);

}


static void
start_button_clicked(GtkWidget *window, gpointer data)
{

	timmer_running = TRUE;
}


static void
stop_button_clicked(GtkWidget *window, gpointer data)
{
	timmer_running = FALSE;
	gtk_label_set_label(GTK_LABEL(lblUpdate), "Stopped" );
}

static void
menu_settings_clicked(GtkMenuItem *menuitem, gpointer data)
{
	GtkBuilder *builder;
	GtkWidget  *window;
	GError     *error = NULL;
	gchar  buf[1024];

	builder = gtk_builder_new();
	if( ! gtk_builder_add_from_file( builder, "src/settings.xml", &error ) )
	{
		g_warning( "%s", error->message );
		g_free( error );
		return;
	}

	window = GTK_WIDGET( gtk_builder_get_object( builder, "window1" ) );

	GtkWidget *btnCancel = GTK_WIDGET(gtk_builder_get_object (builder, "btnCancel"));
	g_signal_connect_swapped (btnCancel, "clicked", G_CALLBACK (gtk_widget_destroy), window);

	GtkWidget *btnSave = GTK_WIDGET(gtk_builder_get_object (builder, "btnSave"));
	g_signal_connect_swapped (btnSave, "clicked", G_CALLBACK (on_btnSave_clicked), window);

	txtServerPort = GTK_WIDGET(gtk_builder_get_object (builder, "txtServerPort"));
	gtk_entry_set_text(GTK_ENTRY(txtServerPort), server_port);

	txtUpdateInterval = GTK_WIDGET(gtk_builder_get_object (builder, "txtUpdateInterval"));
	g_ascii_dtostr(buf, sizeof (buf), update_interval);
	gtk_entry_set_text(GTK_ENTRY(txtUpdateInterval), buf);

	g_object_unref( G_OBJECT( builder ) );
	gtk_widget_show( window );
}

static void
menu_exit_clicked(GtkMenuItem *menuitem, gpointer data)
{
	gtk_main_quit();
}

static void
menu_about_clicked(GtkMenuItem *menuitem, gpointer data)
{
	GtkBuilder *builder;
	GtkWidget  *window;
	GError     *error = NULL;

	builder = gtk_builder_new();
	if( ! gtk_builder_add_from_file( builder, "src/about.xml", &error ) )
	{
		g_warning( "%s", error->message );
		g_free( error );
		return;
	}
	window = GTK_WIDGET( gtk_builder_get_object( builder, "WebWatch" ) );

	g_signal_connect_swapped (window, "response", G_CALLBACK (gtk_widget_destroy), window);

	g_object_unref( G_OBJECT( builder ) );
	gtk_widget_show( window );

}

static void
on_btnSave_clicked(GtkWidget *button, gpointer userdata) {


	gint server_port_int = g_ascii_strtod(gtk_entry_get_text(GTK_ENTRY(txtServerPort)), NULL);
	if( (server_port_int < 80) || (server_port_int > 1024))
	{
		GtkWidget *failDialog = gtk_message_dialog_new(NULL,
	     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
	     		GTK_BUTTONS_OK,
	     		"Bad server port entered. Please enter a value between 80 and 1024\n");
		gtk_dialog_run (GTK_DIALOG (failDialog));
		gtk_widget_destroy (failDialog);
		return;
	}

	gint update_interval_int = g_ascii_strtod(gtk_entry_get_text(GTK_ENTRY(txtUpdateInterval)), NULL);
	if( (update_interval_int < 1) || (update_interval_int > 1000000) )
	{
		GtkWidget *failDialog = gtk_message_dialog_new(NULL,
	     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
	     		GTK_BUTTONS_OK,
	     		"Bad update interval entered. Please enter a value between 1 and 1000000\n");
		gtk_dialog_run (GTK_DIALOG (failDialog));
		gtk_widget_destroy (failDialog);
		return;
	}

	stop_webserver();
	save_settings();
	load_settings();
	start_webserver();
	gtk_widget_destroy(button);
}


static void
refresh_button_clicked(GtkWidget *button, gpointer data)
{

	take_photo();
	sleep(1);
	update_image();
}

void
update_image()
{

	if (!g_file_test (image_file, G_FILE_TEST_EXISTS))
		return;

	gtk_image_set_from_file(GTK_IMAGE(image), image_file);

	GdkPixbuf *pixbuf =	gtk_image_get_pixbuf(GTK_IMAGE(image));
	if (pixbuf == NULL)
	{
		g_printerr("Failed to display image\n");
		return;
	}

	pixbuf = gdk_pixbuf_scale_simple(pixbuf, 300, 300, GDK_INTERP_BILINEAR);

	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);

}

void
load_settings()
{
	  GConfClient* gcClient = NULL;
	  GConfValue* val = NULL;

	  gcClient = gconf_client_get_default();
	  g_assert(GCONF_IS_CLIENT(gcClient));

	  gchar *port_path = g_strconcat(GC_ROOT, "port", NULL);
	  val = gconf_client_get_without_default(gcClient, port_path, NULL);
	  if (val == NULL) {
		  g_warning("Unable to read server port value\n");
	  } else {
		  if (val->type == GCONF_VALUE_STRING) {
			  server_port = g_strndup(gconf_value_get_string(val),STRING_MAX_SIZE - 1);
		  } else {
			  g_warning("Bad server port value set\n");
		  }
	  }

	  gchar *update_path = g_strconcat(GC_ROOT, "update", NULL);
	  val = gconf_client_get_without_default(gcClient, update_path, NULL);
	  if (val == NULL) {
		  g_warning("Unable to read update interval value\n");
	  } else {
		  if (val->type == GCONF_VALUE_INT) {
			  update_interval = gconf_value_get_int(val);
		  } else {
			  g_warning("Bad update interval set\n");
		  }
	  }

	  /*
	  gconf_value_free(val);
	  val = NULL;
	*/
}

void save_settings()
{
	  GConfClient* gcClient = NULL;

	  gcClient = gconf_client_get_default();
	  g_assert(GCONF_IS_CLIENT(gcClient));

	  gchar *port_path = g_strconcat(GC_ROOT, "port", NULL);
	  const gchar *save_server_port = gtk_entry_get_text(GTK_ENTRY(txtServerPort));
	  if (! gconf_client_set_string(gcClient, port_path, save_server_port, NULL) ) {
			GtkWidget *failDialog = gtk_message_dialog_new(NULL,
		     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
		     		GTK_BUTTONS_OK,
		     		"Failed to update server port setting\n");
			gtk_dialog_run (GTK_DIALOG (failDialog));
			gtk_widget_destroy (failDialog);
	  }

	  gchar *update_path = g_strconcat(GC_ROOT, "update", NULL);
	  const gchar *save_update_interval = gtk_entry_get_text(GTK_ENTRY(txtUpdateInterval));
	  if (! gconf_client_set_int(gcClient, update_path, g_ascii_strtod(save_update_interval, NULL), NULL) ) {
			GtkWidget *failDialog = gtk_message_dialog_new(NULL,
		     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
		     		GTK_BUTTONS_OK,
		     		"Failed to update update interval setting\n");
			gtk_dialog_run (GTK_DIALOG (failDialog));
			gtk_widget_destroy (failDialog);
	  }

	  g_object_unref(gcClient);
	  gcClient = NULL;

}

void *count_down_thread ()
{
	gchar  buf[1024];
	int countdown = update_interval;

	for (;;)
	{

		sleep(1);

		if(timmer_running == TRUE)
		{
			gdk_threads_enter ();

			if(countdown == 1)
			{
				take_photo();
				sleep(1);
				update_image();

				countdown = update_interval;

			} else
				countdown--;

			g_ascii_dtostr(buf, sizeof (buf), countdown);
			gtk_label_set_label(GTK_LABEL(lblUpdate), buf );

			gdk_threads_leave ();


		} else {
			countdown = update_interval;
		}

	}

	return NULL;

}

gboolean
start_webserver()
{
	gchar *arg[3] = {web_server_script, "start", NULL};
	gboolean retval;

	if(update_http_config() != TRUE)
		return FALSE;

	retval = g_spawn_sync (app_dir,
							arg, NULL,
			                G_SPAWN_STDERR_TO_DEV_NULL,
			                NULL, NULL, NULL, NULL, NULL, NULL);
	sleep(1);

	if ( retval != TRUE)
		return FALSE;
	else
		return TRUE;


}

gboolean
stop_webserver()
{

	gchar *arg[3] = {web_server_script, "stop", NULL};
	gboolean retval;

	retval = g_spawn_sync (app_dir,
							arg, NULL,
			                G_SPAWN_STDERR_TO_DEV_NULL,
			                NULL, NULL, NULL, NULL, NULL, NULL);
	sleep(1);

	if ( retval != TRUE)
		return FALSE;
	else
		return TRUE;

}

gboolean
update_http_config()
{
	GString *filename;
	FILE *out;

	filename = g_string_new(g_build_filename(config_dir, web_config, NULL));

	if (g_file_test (filename->str, G_FILE_TEST_EXISTS))
	{
		if(g_unlink(filename->str) != 0)
		{
			g_warning("Unable to update web server config file%s\n", filename->str);
			GtkWidget *failDialog = gtk_message_dialog_new(NULL,
								GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
								GTK_BUTTONS_OK,
								"Unable to update web server config file\n");
			gtk_dialog_run (GTK_DIALOG (failDialog));
			gtk_widget_destroy (failDialog);
			return FALSE;
		}
	}

	out = g_fopen(filename->str, "w");
	if (out == NULL)
	{
		GtkWidget *failDialog = gtk_message_dialog_new(NULL,
				     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
				     		GTK_BUTTONS_OK,
				     		"Unable to create web server config file\n");
		gtk_dialog_run (GTK_DIALOG (failDialog));
		gtk_widget_destroy (failDialog);
	    return FALSE;
	}

	fprintf(out, "dir=%s\n", web_server_dir);
	fprintf(out, "cgipat=**.cgi\n");
	fprintf(out, "logfile=%s\n",web_log_file);
	fprintf(out, "pidfile=/var/run/thttpd.pid\n");
	fprintf(out, "port=%s\n",server_port);

	fclose(out);
	sleep(1);
	return TRUE;
}
