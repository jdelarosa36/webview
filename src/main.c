#include <gtk/gtk.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon-banner.h>
#include <gconf/gconf-client.h>
#include <glib/gstdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <glib.h>
#include "camera.h"

/*
#define DEV 1
*/

#define STRING_MAX_SIZE 4096
gboolean timmer_running = FALSE;

GtkWidget *txtServerPort;
GtkWidget *txtUpdateInterval;
GtkWidget *lblUpdate;
GtkWidget *image;

typedef struct _SettingsData SettingsData;
struct _SettingsData {
	int update_interval;
	gchar *server_port;
};
SettingsData *settings;

typedef struct _ConfigData ConfigData;
struct _ConfigData {
	gchar *base_url;
	gchar *image_file;
	gchar *app_dir;
	gchar *web_server_script;
	gchar *web_server_dir;
	gchar *config_dir;
	gchar *web_config;
	gchar *web_log_file;
	const gchar *GC_ROOT;
	gchar *settings_dialog;
	gchar *missing_image_file;
	gchar *about_dialog;
};
ConfigData *config;


typedef struct _AppData AppData;
struct _AppData {
    HildonProgram *program;
    HildonWindow *window;
    GtkWidget *main_toolbar;
    GtkWidget *image;
};
AppData *appdata;

void build_interface();
static void create_menu();

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

void load_settings();
void save_settings();
void initApp();

int main(int argc, char *argv[]) {
	pthread_t 		thread;

	/* init threads */
    g_thread_init (NULL);
    gdk_threads_init ();
    gdk_threads_enter ();

    gtk_init(&argc, &argv);

    initApp();

    appdata->program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("Webview");

    appdata->window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(appdata->program, appdata->window);


    create_menu(appdata->window);
    build_interface(appdata->window);

    /* Connect signal to X in the upper corner */
    g_signal_connect(G_OBJECT(appdata->window), "delete_event", G_CALLBACK(destroy), NULL);

	update_image();
	load_settings();

	gtk_widget_show_all(GTK_WIDGET(appdata->window));

	if(! start_camera(appdata->window))
	{
		g_warning("Unable to start camera\n");

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
	sleep(1);

	pthread_create(&thread, NULL, count_down_thread, NULL);

	gtk_main();

	gdk_threads_leave ();
	return( 0 );
}

void initApp()
{
	gchar *webview_dir;
	gchar *app_dir;

	appdata 						= g_new0(AppData, 1);
	settings 						= g_new0(SettingsData, 1);
	config 							= g_new0(ConfigData, 1);

	settings->update_interval 		= 120;
	settings->server_port 			= "8080";

#ifdef DEV
	webview_dir = ".";
	app_dir = ".";
#else
	webview_dir = "/usr/var/webview";
	app_dir = "/usr/bin";
#endif

	config->base_url 				= "http://localhost:";
	config->image_file	 			= g_build_filename(webview_dir, "/website/Picture.jpg", NULL);
	config->app_dir 				= g_build_filename(webview_dir, "/config/", NULL);
	config->web_server_script 		= "webview.d";
	config->web_server_dir 			= g_build_filename(webview_dir, "/website/", NULL);
	config->config_dir 				= g_build_filename(webview_dir, "/config/", NULL);
	config->web_log_file 			= g_build_filename(webview_dir, "/logs/thttpd_log", NULL);
	config->GC_ROOT 				= "/apps/gladetest/";
	config->settings_dialog			= g_build_filename(webview_dir, "/dialogs/settings.xml", NULL);
	config->missing_image_file		= g_build_filename(webview_dir, "/website/missing_image.png", NULL);
	config->about_dialog			= g_build_filename(webview_dir, "/dialogs/about.xml", NULL);

}

void
destroy (GtkWidget *widget, gpointer data)
{
	stop_webserver();
	end_camera();
    gtk_main_quit();
}

void
set_open_web_button(GtkWidget *web_link_button)
{
	gchar *web_url = g_strconcat(config->base_url, settings->server_port, NULL);
	gtk_link_button_set_uri(GTK_LINK_BUTTON(web_link_button), web_url);

}


static void
start_button_clicked(GtkWidget *window, gpointer data)
{

	if(timmer_running != TRUE)
	{

	hildon_banner_show_information(GTK_WIDGET(appdata->window),
			NULL, "Starting Countdown Timer");
		timmer_running = TRUE;
	}
}


static void
stop_button_clicked(GtkWidget *window, gpointer data)
{
	if(timmer_running != FALSE)
	{
		hildon_banner_show_information(GTK_WIDGET(appdata->window),
				NULL, "Stopping Countdown Timer");

		timmer_running = FALSE;
		gtk_label_set_label(GTK_LABEL(lblUpdate), "Stopped" );
	}
}

static void
menu_settings_clicked(GtkMenuItem *menuitem, gpointer data)
{
	GtkBuilder *builder;
	GtkWidget  *window;
	GError     *error = NULL;
	gchar  buf[1024];

	builder = gtk_builder_new();
	if( ! gtk_builder_add_from_file( builder, config->settings_dialog, &error ) )
	{
		g_warning( "%s", error->message );
		g_free( error );
		return;
	}

	window = GTK_WIDGET( gtk_builder_get_object( builder, "dialog1" ) );

	GtkWidget *btnCancel = GTK_WIDGET(gtk_builder_get_object (builder, "btnCancel"));
	g_signal_connect_swapped (btnCancel, "clicked", G_CALLBACK (gtk_widget_destroy), window);

	GtkWidget *btnSave = GTK_WIDGET(gtk_builder_get_object (builder, "btnSave"));
	g_signal_connect_swapped (btnSave, "clicked", G_CALLBACK (on_btnSave_clicked), window);

	txtServerPort = GTK_WIDGET(gtk_builder_get_object (builder, "txtServerPort"));
	gtk_entry_set_text(GTK_ENTRY(txtServerPort), settings->server_port);

	txtUpdateInterval = GTK_WIDGET(gtk_builder_get_object (builder, "txtUpdateInterval"));
	g_ascii_dtostr(buf, sizeof (buf), settings->update_interval);
	gtk_entry_set_text(GTK_ENTRY(txtUpdateInterval), buf);

	g_object_unref( G_OBJECT( builder ) );
	gtk_widget_show( window );
}

static void
menu_exit_clicked(GtkMenuItem *menuitem, gpointer data)
{
	destroy(NULL, NULL);
}

static void
menu_about_clicked(GtkMenuItem *menuitem, gpointer data)
{
	GtkBuilder *builder;
	GtkWidget  *window;
	GError     *error = NULL;

	builder = gtk_builder_new();
	if( ! gtk_builder_add_from_file( builder, config->about_dialog, &error ) )
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
	if( (server_port_int < 1024) || (server_port_int > 32766) )
	{
			GtkWidget *failDialog = gtk_message_dialog_new(NULL,
		     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
		     		GTK_BUTTONS_OK,
		     		"Bad server port value.\nServer port must be between 1024 and 32766.");
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

	take_photo(config->image_file);
	sleep(1);
	update_image();
}

void
update_image()
{

	if (!g_file_test (config->image_file, G_FILE_TEST_EXISTS))
	{
		g_warning( "Missing Image file: %s", config->image_file);
		return;
	}

	gtk_image_set_from_file(GTK_IMAGE(image), config->image_file);

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
	  gdouble sport;

	  gcClient = gconf_client_get_default();
	  g_assert(GCONF_IS_CLIENT(gcClient));

	  gchar *port_path = g_strconcat(config->GC_ROOT, "port", NULL);
	  val = gconf_client_get_without_default(gcClient, port_path, NULL);
	  if (val == NULL) {
		  g_warning("Unable to read server port value\n");
	  } else {
		  if (val->type == GCONF_VALUE_STRING) {
			  settings->server_port = g_strndup(gconf_value_get_string(val),STRING_MAX_SIZE - 1);

			  sport = g_ascii_strtod(settings->server_port, NULL);
			  if( (sport < 1024) || (sport > 32766) )
			  {
					GtkWidget *failDialog = gtk_message_dialog_new(NULL,
				     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
				     		GTK_BUTTONS_OK,
				     		"Bad server port value.\nServer port must be between 1024 and 32766.\nDefaulting to 8080.");
					gtk_dialog_run (GTK_DIALOG (failDialog));
					gtk_widget_destroy (failDialog);
					settings->server_port = "8080";
			  }

		  } else {
			  g_warning("Bad server port value set\n");
		  }
	  }

	  gchar *update_path = g_strconcat(config->GC_ROOT, "update", NULL);
	  val = gconf_client_get_without_default(gcClient, update_path, NULL);
	  if (val == NULL) {
		  g_warning("Unable to read update interval value\n");
	  } else {
		  if (val->type == GCONF_VALUE_INT) {
			  settings->update_interval = gconf_value_get_int(val);
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

	  gchar *port_path = g_strconcat(config->GC_ROOT, "port", NULL);
	  const gchar *save_server_port = gtk_entry_get_text(GTK_ENTRY(txtServerPort));
	  if (! gconf_client_set_string(gcClient, port_path, save_server_port, NULL) ) {
			GtkWidget *failDialog = gtk_message_dialog_new(NULL,
		     		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
		     		GTK_BUTTONS_OK,
		     		"Failed to update server port setting\n");
			gtk_dialog_run (GTK_DIALOG (failDialog));
			gtk_widget_destroy (failDialog);
	  }

	  gchar *update_path = g_strconcat(config->GC_ROOT, "update", NULL);
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
	int countdown = settings->update_interval;

	for (;;)
	{

		sleep(1);

		if(timmer_running == TRUE)
		{
			gdk_threads_enter ();

			if(countdown == 1)
			{
				take_photo(config->image_file);
				sleep(1);
				update_image();

				countdown = settings->update_interval;

			} else
				countdown--;

			g_ascii_dtostr(buf, sizeof (buf), countdown);
			gtk_label_set_label(GTK_LABEL(lblUpdate), buf );

			gdk_threads_leave ();


		} else {
			countdown = settings->update_interval;
		}

	}

	return NULL;

}

gboolean
start_webserver()
{

	hildon_banner_show_information(
			GTK_WIDGET(appdata->window),
			NULL,
			"Starting Webserver");

	gchar *arg[4] = {config->web_server_script, "start", settings->server_port, NULL};
	gboolean retval;

	retval = g_spawn_sync (config->app_dir,
							arg, NULL,
			                G_SPAWN_STDERR_TO_DEV_NULL,
			                NULL, NULL, NULL, NULL, NULL, NULL);
	sleep(1);

	if (retval != TRUE)
		return FALSE;
	else
		return TRUE;


}

gboolean
stop_webserver()
{

	hildon_banner_show_information(
			GTK_WIDGET(appdata->window),
			NULL,
			"Stopping Webserver");

	gchar *arg[3] = {config->web_server_script, "stop", NULL};
	gboolean retval;

	retval = g_spawn_sync (config->app_dir,
							arg, NULL,
			                G_SPAWN_STDERR_TO_DEV_NULL,
			                NULL, NULL, NULL, NULL, NULL, NULL);
	sleep(1);

	if ( retval != TRUE)
		return FALSE;
	else
		return TRUE;

}

void build_interface()
{

	GtkWidget *fixed = gtk_fixed_new();
	gtk_container_add(GTK_CONTAINER(appdata->window), fixed);
	gtk_widget_show(fixed);

	/*  GtkImage */
	image = gtk_image_new();
	if (g_file_test (config->missing_image_file, G_FILE_TEST_EXISTS))
	{
		gtk_image_set_from_file(GTK_IMAGE(image), config->missing_image_file);
	}else{
		g_warning("unable to load inital image file %s\n", config->missing_image_file);
	}
	gtk_fixed_put(GTK_FIXED(fixed), image, 10, 10);
	GdkPixbuf *pixbuf =	gtk_image_get_pixbuf(GTK_IMAGE(image));
	pixbuf = gdk_pixbuf_scale_simple(pixbuf, 300, 300, GDK_INTERP_BILINEAR);
	gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
	gtk_widget_show(image);

	/* GtkLabel */
	GtkWidget *label1 = gtk_label_new("Next Update:");
	gtk_fixed_put(GTK_FIXED(fixed), label1, 450, 100);
	gtk_widget_show(label1);

	/* GtkLabel */
	lblUpdate = gtk_label_new("Stopped");
	gtk_fixed_put(GTK_FIXED(fixed), lblUpdate, 590, 100);
	gtk_widget_show(lblUpdate);

	/* GtkButton */
	GtkWidget *btnStart = gtk_button_new_with_label("Start");
	gtk_fixed_put(GTK_FIXED(fixed), btnStart, 450, 160);
	gtk_widget_set_size_request (btnStart, 80, 40);
	gtk_widget_show(btnStart);
    g_signal_connect (G_OBJECT (btnStart), "clicked", G_CALLBACK (start_button_clicked), NULL);

	/* GtkButton */
	GtkWidget *btnStop = gtk_button_new_with_label("Stop");
	gtk_fixed_put(GTK_FIXED(fixed), btnStop, 545, 160);
	gtk_widget_set_size_request (btnStop, 80, 40);
	gtk_widget_show(btnStop);
    g_signal_connect (G_OBJECT (btnStop), "clicked", G_CALLBACK (stop_button_clicked), NULL);

	/* GtkButton */
	GtkWidget *btnRefresh = gtk_button_new_with_label("Refresh");
	gtk_fixed_put(GTK_FIXED(fixed), btnRefresh, 115, 325);
	gtk_widget_set_size_request (btnRefresh, 100, 40);
	gtk_widget_show(btnRefresh);
	g_signal_connect (G_OBJECT (btnRefresh), "clicked", G_CALLBACK (refresh_button_clicked), NULL);

}

static void create_menu()
{
    /* Create needed variables */
    GtkWidget *main_menu;
    GtkWidget *item_settings;
    GtkWidget *item_about;
    GtkWidget *item_separator;
    GtkWidget *item_close;

    /* Create new main menu */
    main_menu = gtk_menu_new();

    /* Create menu items */
    item_settings = gtk_menu_item_new_with_label("Settings");
    item_about = gtk_menu_item_new_with_label("About");
    item_separator = gtk_separator_menu_item_new();
    item_close = gtk_menu_item_new_with_label("Close");

    /* Add menu items to right menus*/
    gtk_menu_append(main_menu, item_settings);
    gtk_menu_append(main_menu, item_about);
    gtk_menu_append(main_menu, item_separator);
    gtk_menu_append(main_menu, item_close);

    hildon_window_set_menu(appdata->window, GTK_MENU(main_menu));

    /* Attach the callback functions to the activate signal */
    g_signal_connect(G_OBJECT(item_settings), "activate", GTK_SIGNAL_FUNC(menu_settings_clicked), NULL);
    g_signal_connect(G_OBJECT(item_about), "activate", GTK_SIGNAL_FUNC(menu_about_clicked), NULL);
    g_signal_connect(G_OBJECT(item_close), "activate", GTK_SIGNAL_FUNC(menu_exit_clicked), NULL);

    /* Make all menu widgets visible*/
    gtk_widget_show_all(GTK_WIDGET(main_menu));

}
