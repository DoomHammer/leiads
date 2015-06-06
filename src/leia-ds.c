/*
	leia-ds.c
	Copyright (C) 2008-2011 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

	--------------------------------------------------------------------

	This file is part of the Leia application.

	Leia is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Leia is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Leia.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "leia-ds.h"

#ifdef OSSO_SERVICE
#include <libosso.h>
#endif

#ifdef MAEMO
#include <hildon-uri.h>
#include <conic.h>
#endif

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <shellapi.h>        /* for ShellExecute() */
#endif

#include "../icons/16x16/leiads.h"
#ifdef MAEMO
#define ICONS "/usr/share/icons/hicolor"
#else
#include "../icons/26x26/leiads.h"
#include "../icons/32x32/leiads.h"
#include "../icons/40x40/leiads.h"
#include "../icons/48x48/leiads.h"
#include "../icons/64x64/leiads.h"
#endif

#ifdef MAEMO
/*#include "../icons/26x26/qgn_list_gene_music_file.h"*/
extern const guint8 qgn_list_gene_music_file_26x26[];
#else
#include "../icons/16x16/qgn_list_gene_music_file.h"
#include "../icons/24x24/qgn_list_gene_music_file.h"
#endif

/* Configuration data */
#define FILTER_LINN_UPNP_AV_RENDERER

#ifdef MAEMO
#define RUNS_IN_SCRATCHBOX()  (g_getenv( "SBOX_UNAME_MACHINE" ) != NULL)
#define SHOW_DEVICE_MENU      TRUE
#else
#define RUNS_IN_SCRATCHBOX()  FALSE
#define SHOW_DEVICE_MENU      FALSE
#endif

#define STARTUP_WINDOW_WIDTH  800
#define DEFAULT_HPOSITION     200
#define DEFAULT_VPOSITION     200

/*-----------------------------------------------------------------------------------------------*/

#ifdef OSSO_SERVICE
osso_context_t *osso;
const gchar *osso_product_name, *osso_product_hardware;
#endif
#ifdef MAEMO
HildonProgram *Program;
HildonWindow *MainWindow;
#else
GtkWindow *MainWindow;
GtkStatusbar *Statusbar;
guint StatusbarContext;
GtkMenuItem *StatusMenuItem;
#endif

int NumServer, NumRenderer;

static GtkMenuItem *mi_view[NUM_PAGES], *mi_fullscreen, *mi_tools;
static GtkToolItem *ti_notebook[NUM_PAGES];
static GtkNotebook *notebook;
#ifndef MAEMO
static GtkPaned *hpaned, *vpaned;
#endif
static GtkWidget *browser, *playlist, *now_playing;
#ifdef CON_IC_CONNECTION  /* conic.h was included */
static ConIcConnection *connection;
static struct animation *connection_animation;
#endif
static gboolean connected, quitting;

static GtkMenu *m_renderer;
static const char upnp_device_key[] = "upnp_device";
static int current_page_num = -1;

GtkMenuShell *create_menu( void );
void update_view_menu( void );
void update_tools_menu( void );
void update_tools_menu2( GtkMenuShell *tools_menu );
GtkToolbar *create_toolbar( void );
void set_fullscreen( gboolean active );

gboolean status_connected( gpointer user_data );
gpointer search_thread( gpointer user_data );
gboolean searching_1( gpointer user_data );
gboolean searching_2( gpointer user_data );
gboolean end_of_search( gpointer user_data );
gboolean status_disconnected( gpointer user_data );
void close_all( void );

gboolean open_renderer( const upnp_device *device );
void close_renderer( gboolean standby );
gboolean renderer_callback( upnp_subscription *subscription, char *e_propertyset );
gboolean renderer_callback_idle( gpointer data );

void about( void );             /* (GtkMenuItem *menuitem, gpointer user_data) */
void debug_screenshot( void );  /* (GtkMenuItem *menuitem, gpointer user_data) */

void fullscreen_toggled( GtkCheckMenuItem *check_menu_item, gpointer user_data );
void view_toggled( GtkCheckMenuItem *check_menu_item, gpointer user_data );
void notebook_toggled( GtkToggleToolButton *toggle_tool_button, gpointer user_data );
void page_switched( GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data );
void renderer_toggled( GtkMenuItem *menu_item, gpointer user_data );

#ifdef CON_IC_CONNECTION
void connection_event( ConIcConnection *connection, ConIcConnectionEvent *event, gpointer user_data );
#endif
void con_ic_status_connected( void );
void con_ic_status_disconnecting( void );
void con_ic_status_disconnected( void );

#ifndef MAEMO
guint clock_timeout_id;
gboolean show_clock;
gboolean clock_timer( gpointer user_data );
gboolean main_window_state_event( GtkWidget *widget, GdkEventWindowState *event, gpointer user_data );
#endif

/*-----------------------------------------------------------------------------------------------*/

static gboolean get_boolean_value( const char *arg )
{
	arg = strchr( arg, '=' );
	if ( arg != NULL )
	{
		arg++;
		return (*arg == '1' || *arg == 't' || *arg == 'y');
	}

	return TRUE;
}
static int get_int_value( const char *arg, int default_value )
{
	arg = strchr( arg, '=' );
	if ( arg != NULL )
	{
		arg++;
		return atoi( arg );
	}

	return default_value;
}

static void append_info( GString *str, const char *name, const char *info )
{
	g_string_append_printf( str, "\n%s = %s", name, (info != NULL) ? info : "<NULL>" );
}

static void append_env_info( GString *str, const char *name )
{
	const char *info = g_getenv( name );
	if ( info != NULL ) append_info( str, name, info );
}

/*-----------------------------------------------------------------------------------------------*/

static gboolean leiads_info( gpointer data )
{
	GString *str = g_string_new( NAME " " VERSION " (" __DATE__ " " __TIME__ ")\n" );

	g_string_append_printf( str, "\nGTK %u.%u.%u, GLib %u.%u.%u\n",
		gtk_major_version, gtk_minor_version, gtk_micro_version,
		glib_major_version, glib_minor_version, glib_micro_version );

	append_info( str, "g_get_application_name()", g_get_application_name() );
	append_info( str, "g_get_prgname()", g_get_prgname() );
	append_info( str, "g_get_user_name()", g_get_user_name() );
	append_info( str, "g_get_real_name()", g_get_real_name() );
	append_info( str, "g_get_host_name()", g_get_host_name() );

	g_string_append( str, "\n" );

	append_env_info( str, "DESKTOP_SESSION" );

	gdk_threads_enter();
	message_dialog( GTK_MESSAGE_INFO, str->str );
	gdk_threads_leave();

	g_string_free( str, TRUE );
	return FALSE;
}

static gboolean leiads_dirs( gpointer data )
{
	GString *str = g_string_new( NAME " " VERSION " (" __DATE__ " " __TIME__ ")\n" );

	/*append_info( str, "g_get_current_dir()", g_get_current_dir() );*/
	append_info( str, "g_get_user_cache_dir()", g_get_user_cache_dir() );
	append_info( str, "g_get_user_data_dir()", g_get_user_data_dir() );
	append_info( str, "g_get_user_config_dir()", g_get_user_config_dir() );
	append_info( str, "g_get_home_dir()", g_get_home_dir() );
	append_info( str, "g_get_tmp_dir()", g_get_tmp_dir() );

	gdk_threads_enter();
	message_dialog( GTK_MESSAGE_INFO, str->str );
	gdk_threads_leave();

	g_string_free( str, TRUE );
	return FALSE;
}

#ifdef OSSO_SERVICE
static gboolean leiads_osso( gpointer data )
{
	GString *str = g_string_new( NAME " " VERSION " (" __DATE__ " " __TIME__ ")\n" );

	append_env_info( str, "OSSO_VERSION" );
	append_env_info( str, "OSSO_PRODUCT_RELEASE_NAME" );
	append_env_info( str, "OSSO_PRODUCT_RELEASE_VERSION" );
	append_env_info( str, "OSSO_PRODUCT_NAME" );
	append_env_info( str, "OSSO_PRODUCT_FULL_NAME" );
	append_env_info( str, "OSSO_PRODUCT_SHORT_NAME" );
	append_env_info( str, "OSSO_PRODUCT_HARDWARE" );

	gdk_threads_enter();
	message_dialog( GTK_MESSAGE_INFO, str->str );
	gdk_threads_leave();

	g_string_free( str, TRUE );
	return FALSE;
}
#endif

static gboolean initialisation_failed( gpointer data )
{
	gchar *msg = (gchar *)data;

	gdk_threads_enter();
	message_dialog( GTK_MESSAGE_ERROR, msg );
	Quit();
	gdk_threads_leave();

	g_free( msg );
	return FALSE;
}

/* Main application */

static const char leiads_ini[] = "leiads.ini";

#ifdef WIN32
int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow )
#define argc __argc
#define argv __argv
#else
int main( int argc, char *argv[] )
#endif
{
	gboolean fullscreen, maximize, iconify;
	gint window_width, window_height;
	gboolean success = TRUE;

	/* Initialize the GTK. */
	g_thread_init( NULL );
	gdk_threads_init();
	gtk_init( &argc, &argv );

#if LOGLEVEL

	debug_init();
	g_print( NAME " " VERSION " (" CODENAME ") [" __DATE__ " / " __TIME__ "]\n" );
#if defined( WIN32 )
{
	OSVERSIONINFO vi = {0};
	vi.dwOSVersionInfoSize = sizeof( vi );
	GetVersionEx( &vi );
	g_print( "Win32 %lu.%lu.%lu %s\n", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber, vi.szCSDVersion );
}
#elif defined( MAEMO )
	g_print( "Maemo\n" );
#else
	g_print( "Linux\n" );
#endif
	g_print( "GTK %u.%u.%u\n", gtk_major_version, gtk_minor_version, gtk_micro_version );
	g_print( "GLib %u.%u.%u\n", glib_major_version, glib_minor_version, glib_micro_version );
	g_print( "sizeof( short ) = %lu, sizeof( int ) = %lu, sizeof( long ) = %lu\n", sizeof( short ), sizeof( int ), sizeof( long ) );

#endif

#ifdef OSSO_SERVICE

	/* OSSO initialize */
	/*TRACE(( "osso_initialize( \"" OSSO_SERVICE "\", \"" VERSION "\", FALSE, NULL )\n" ));*/
	osso = osso_initialize( OSSO_SERVICE, VERSION, FALSE, NULL );
	osso_product_name = g_getenv( "OSSO_PRODUCT_NAME" );
	if ( osso_product_name == NULL ) osso_product_name = "?";
	osso_product_hardware = g_getenv( "OSSO_PRODUCT_HARDWARE" );
	if ( osso_product_hardware == NULL ) osso_product_hardware = "?";
	/*
		OSSO_VERSION                 = "RX-34+RX-44+RX-48_DIABLO_4.2008.36-5_PR_MR0"
		OSSO_PRODUCT_RELEASE_NAME    = "OS 2008"
		OSSO_PRODUCT_RELEASE_VERSION = "4.2008.36-5"
		OSSO_PRODUCT_NAME            = "N800"
		OSSO_PRODUCT_FULL_NAME       = "Nokia N800 Internet Tablet"
		OSSO_PRODUCT_SHORT_NAME      = "Nokia N800"
		OSSO_PRODUCT_HARDWARE        = "RX-34"
	*/

#endif

#ifdef MAEMO

	/* Create the hildon program */
	Program = HILDON_PROGRAM( hildon_program_get_instance() );

#endif

	/* Setup the application */
	g_set_application_name( NAME );

#ifdef MAEMO

	/* Create HildonWindow and set it to HildonProgram */
	MainWindow = HILDON_WINDOW( hildon_window_new() );
	hildon_program_add_window( Program, MainWindow );

#else

	/* Create GtkWindow as top level window */
	MainWindow = GTK_WINDOW( gtk_window_new( GTK_WINDOW_TOPLEVEL ) );
	set_main_window_title( NULL );

	/* Use LeiaDS icon as main icon */
#ifndef WIN32
{
	GList *list = NULL;

	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_16x16, FALSE, NULL ) );
	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_26x26, FALSE, NULL ) );
	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_32x32, FALSE, NULL ) );
	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_40x40, FALSE, NULL ) );
	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_48x48, FALSE, NULL ) );
	list = g_list_append( list, gdk_pixbuf_new_from_inline( -1, leiads_icon_64x64, FALSE, NULL ) );

	gtk_window_set_default_icon_list( list );
	/*gtk_window_set_icon_list( MainWindow, list );*/
}
#endif

#endif

	LoadSettings();

#ifndef MAEMO
	window_width = ( Settings->startup.classic_ui )
		? GetNowPlayingWidth( FALSE, Settings->now_playing.album_art_size )
		: STARTUP_WINDOW_WIDTH;
	window_height = (window_width * 3) / 4;
#endif

{	/* Parse command line arguments */

	const char *lang = NULL, *share = NULL;
	gchar *lang_buf;
	int i;

	fullscreen = Settings->startup.fullscreen;
	maximize = Settings->startup.maximize;
	iconify = FALSE;

	for ( i = 1; i < argc; i++ )
	{
		const char *arg = argv[i];
		if ( *arg == '-' || *arg == '/' ) arg++;

		if ( strncmp( arg, "fullscreen", 10 ) == 0 )
			fullscreen = get_boolean_value( arg );
		else if ( strncmp( arg, "iconify", 7 ) == 0 )
			iconify = get_boolean_value( arg );
		else if ( strncmp( arg, "maximize", 8 ) == 0 )
			maximize = get_boolean_value( arg );
		else if ( strncmp( arg, "minimize", 8 ) == 0 )
			iconify = get_boolean_value( arg );

		else if ( strncmp( arg, "lang=", 5 ) == 0 )
			lang = arg + 5;
		else if ( strncmp( arg, "share=", 6 ) == 0 )
			share = arg + 6;
		else if ( strncmp( arg, "MID", 3 ) == 0 )
			Settings->MID = get_boolean_value( arg );

		else if ( strncmp( arg, "keybd", 5 ) == 0 )
			Settings->debug.keybd = get_boolean_value( arg );
		else if ( strncmp( arg, "screenshot", 10 ) == 0 )
			Settings->debug.screenshot = get_int_value( arg, 5 );

		else if ( strcmp( arg, "info" ) == 0 )
			g_idle_add( leiads_info, NULL );
		else if ( strcmp( arg, "dirs" ) == 0 )
			g_idle_add( leiads_dirs, NULL );
	#ifdef OSSO_SERVICE
		else if ( strcmp( arg, "osso" ) == 0 )
			g_idle_add( leiads_osso, NULL );
	#endif

		else if ( sscanf( arg, "%dx%d", &window_width, &window_height ) < 2 )
		{
			gchar *msg = g_strdup_printf( "Unrecognized option `%s'", arg );
			g_printerr( "leiads: %s\n", msg );
			g_idle_add( initialisation_failed, msg );
			success = FALSE;
		}
	}

#ifndef MAEMO
	TRACE(( "gtk_window_set_default_size( MainWindow, %d, %d )\n", window_width, window_height ));
	gtk_window_set_default_size( MainWindow, window_width, window_height );
#endif
	if ( fullscreen ) gtk_window_fullscreen( GTK_WINDOW(MainWindow) );
	if ( maximize ) gtk_window_maximize( GTK_WINDOW(MainWindow) );
	if ( iconify ) gtk_window_iconify( GTK_WINDOW(MainWindow) );

	if ( share != NULL ) SetShareDir( share );

	lang_buf = SetLanguage( lang );
	if ( lang_buf == NULL )
	{
		lang_buf = SetLanguage( "en" );
		if ( lang_buf == NULL )
		{
			gchar *msg = g_strdup( "Language file `leiads_??.txt' not found" );
			g_printerr( "leiads: %s\n", msg );
			g_idle_add( initialisation_failed, msg );
			success = FALSE;
		}
	}
}

{	/* Parse resource file */

	gchar *pathname = BuildApplDataFilename( "gtkrc" );
	gtk_rc_parse( pathname );
	g_free( pathname );
}

#ifdef MAEMO

	/* Create menu */
	hildon_window_set_menu( MainWindow, GTK_MENU(create_menu()) );

	/* Create toolbar */
	hildon_window_add_toolbar( MainWindow, GTK_TOOLBAR(create_toolbar()) );

	/* Create notebook */
	notebook = GTK_NOTEBOOK(gtk_notebook_new());
	gtk_notebook_set_show_tabs( notebook, FALSE );
	gtk_notebook_set_show_border( notebook, FALSE );
	gtk_notebook_append_page( notebook, browser = CreateBrowser(), NULL );
	gtk_notebook_append_page( notebook, playlist = CreatePlaylist(), NULL );
	gtk_notebook_append_page( notebook, now_playing = CreateNowPlaying(), NULL );
	g_signal_connect( G_OBJECT(notebook), "switch-page", G_CALLBACK(page_switched), NULL );

	gtk_container_add( GTK_CONTAINER( MainWindow ), GTK_WIDGET(notebook) );

#else
{
	/* Create vbox */
	GtkBox *vbox = GTK_BOX( gtk_vbox_new( FALSE, 0 ) );

	/* Create menu */
	gtk_box_pack_start( vbox, GTK_WIDGET(create_menu()), FALSE, FALSE, 0 );

	/* Create toolbar */
	(Settings->startup.toolbar_at_bottom ? gtk_box_pack_end : gtk_box_pack_start)
		( vbox, GTK_WIDGET(create_toolbar()), FALSE, FALSE, 0 );

	browser = CreateBrowser();
	playlist = CreatePlaylist();
	now_playing = CreateNowPlaying();

	if ( !Settings->startup.classic_ui )
	{
		gchar *filename = BuildUserConfigFilename( leiads_ini );
		gchar *data;

		gint hposition = DEFAULT_HPOSITION, vposition = DEFAULT_VPOSITION;

		if ( g_file_get_contents( filename, &data, NULL, NULL ) )
		{
			GKeyFile *key_file = g_key_file_new();

			if ( g_key_file_load_from_data( key_file, data, -1, G_KEY_FILE_NONE, NULL ) )
			{
				hposition = g_key_file_get_integer( key_file, "Paned", "hposition", NULL );
				vposition = g_key_file_get_integer( key_file, "Paned", "vposition", NULL );
			}

			g_key_file_free( key_file );
		}

		g_free( filename );

		hpaned = GTK_PANED( gtk_hpaned_new() );
		vpaned = GTK_PANED( gtk_vpaned_new() );

		gtk_paned_add1( hpaned, browser );
		gtk_paned_add2( hpaned, GTK_WIDGET(vpaned) );
		if ( hposition ) gtk_paned_set_position( hpaned, hposition );

		gtk_paned_add1( vpaned, playlist );
		gtk_paned_add2( vpaned, now_playing );
		if ( vposition ) gtk_paned_set_position( vpaned, vposition );

		gtk_box_pack_start( vbox, GTK_WIDGET(hpaned), TRUE, TRUE, 0 );
	}
	else
	{
		/* Create notebook */
		notebook = GTK_NOTEBOOK(gtk_notebook_new());
		gtk_notebook_set_show_tabs( notebook, FALSE );
		gtk_notebook_set_show_border( notebook, FALSE );
		gtk_notebook_append_page( notebook, browser, NULL );
		gtk_notebook_append_page( notebook, playlist, NULL );
		gtk_notebook_append_page( notebook, now_playing, NULL );
		g_signal_connect( G_OBJECT(notebook), "switch-page", G_CALLBACK(page_switched), NULL );

		gtk_box_pack_start( vbox, GTK_WIDGET(notebook), TRUE, TRUE, 0 );
		gtk_container_set_focus_child( GTK_CONTAINER(vbox), GTK_WIDGET(notebook) );
	}

	if ( !Settings->MID )
	{
		Statusbar = GTK_STATUSBAR(gtk_statusbar_new());
		StatusbarContext = gtk_statusbar_get_context_id( Statusbar, "context-description" );
		gtk_box_pack_end( vbox, GTK_WIDGET(Statusbar), FALSE, FALSE, 0 );
	}

	gtk_container_add( GTK_CONTAINER( MainWindow ), GTK_WIDGET(vbox) );
}
#endif

	/* Connect signal to X in the upper corner */
	g_signal_connect( G_OBJECT(MainWindow), "delete-event", G_CALLBACK(Quit), NULL );

	/* Connect hardware button listener */
	g_signal_connect( G_OBJECT(MainWindow), "key-press-event", G_CALLBACK(KeyPressEvent), GINT_TO_POINTER(-1) );
	g_signal_connect( G_OBJECT(MainWindow), "key-release-event", G_CALLBACK(KeyReleaseEvent), GINT_TO_POINTER(-1) );

	CloseBrowser();
	close_renderer( FALSE );

	/* Begin the main application */
	TRACE(( "gtk_widget_show_all()...\n" ));
	gtk_widget_show_all( GTK_WIDGET(MainWindow) );  /* => "switch-page" event */
	TRACE(( "gtk_widget_show_all()...ok\n" ));

	update_view_menu();
	update_tools_menu();
	if ( fullscreen ) set_fullscreen( fullscreen );

	OnUnselectPlaylist();
	OnUnselectNowPlaying();

	if ( success )
	{
		GError *error = NULL;

		if ( upnp_startup( &error ) )
		{
			if ( RUNS_IN_SCRATCHBOX() )
			{
				TRACE(( "### SCRATCHBOX\n" ));
				SSDP_LOOP = 1;
				con_ic_status_connected();
			}
			else
			{
			#ifdef CON_IC_CONNECTION
				gboolean success;

				SSDP_LOOP = 0;
				connected = FALSE;

				connection_animation = show_animation( Text(WLAN_CONNECTING), NULL, 1000 );

				/* Request for a connection (IAP) */
				connection = con_ic_connection_new();
				g_signal_connect( G_OBJECT( connection ), "connection-event", G_CALLBACK(connection_event), NULL );
				g_object_set( G_OBJECT( connection ), "automatic-connection-events", TRUE, NULL );
				TRACE(( "con_ic_connection_connect()...\n" ));
				success = con_ic_connection_connect( connection, CON_IC_CONNECT_FLAG_NONE );
				TRACE(( "con_ic_connection_connect() = %s\n", (success) ? "TRUE" : "FALSE" ));
				if ( !success ) g_warning( "Request for connection failed" );
			#else
			#ifdef MAEMO
				SSDP_LOOP = 0;
			#else
				SSDP_LOOP = 1;
			#endif
				con_ic_status_connected();
			#endif
			}
		}

		HandleError( error, Text(ERROR_UPNP_STARTUP) );
	}

	gdk_threads_enter();
	gtk_main();
	gdk_threads_leave();

	upnp_cleanup( connected );

#ifdef OSSO_SERVICE
	/* Exit */
	osso_deinitialize( osso );
#endif

#if LOGLEVEL
	debug_exit();
#endif

	return 0;
}

/*-----------------------------------------------------------------------------------------------*/

/* Create the menu items needed for the main view */
static void create_file_menu_items( GtkMenuShell *sub_menu )
{
	GtkWidget *item;

	if ( Settings->debug.screenshot > 0 )
	{
		item = gtk_menu_item_new_with_label( "Take Screenshot..." );
		g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(debug_screenshot), NULL );
		gtk_menu_shell_append( sub_menu, item );

		gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );
	}

	item = gtk_menu_item_new_with_label( Text(MENU_QUIT) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(Quit), NULL );
	gtk_menu_shell_append( sub_menu, item );
}

GtkMenuShell *create_menu( void )
{
#ifdef MAEMO
	GtkMenuShell *menu = GTK_MENU_SHELL(gtk_menu_new());
#else
	GtkMenuShell *menu = GTK_MENU_SHELL(gtk_menu_bar_new());
#endif
	GtkMenuShell *sub_menu;
	GtkWidget *item;

	TRACE(( "-> create_menu()\n" ));

	/* File menu (not on Maemo) */

#ifndef MAEMO

	item = gtk_menu_item_new_with_label( Text(MENU_FILE) );
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, item );

	create_file_menu_items( sub_menu );

#endif

	/* Other menus */

	CreateBrowserMenuItems( menu );
	CreatePlaylistMenuItems( menu );
	CreateNowPlayingMenuItems( menu );
	CreateTransportMenuItems( menu );
	CreateVolumeMenuItems( menu );

	/* View menu */

	item = gtk_menu_item_new_with_label( Text(MENU_VIEW) );
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, item );

	if ( Settings->startup.classic_ui )
	{
		GSList *group = NULL;
		int i;

		for ( i = 0; i < NUM_PAGES; i++ )
		{
			mi_view[i] = GTK_MENU_ITEM(gtk_radio_menu_item_new_with_label( group, Texts[TextIndex(BROWSER)+i] ));
			group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(mi_view[i]) );
			g_signal_connect( G_OBJECT(mi_view[i]), "toggled", G_CALLBACK(view_toggled), GINT_TO_POINTER(i) );
			gtk_menu_shell_append( sub_menu, GTK_WIDGET(mi_view[i]) );
		}

		gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );
	}

	mi_fullscreen = GTK_MENU_ITEM(gtk_check_menu_item_new_with_label( Text(KEYBD_FULLSCREEN) ));
	g_signal_connect( G_OBJECT(mi_fullscreen), "toggled", G_CALLBACK(fullscreen_toggled), NULL );
	gtk_menu_shell_append( sub_menu, GTK_WIDGET(mi_fullscreen) );

	/* Tools menu */

#if SHOW_DEVICE_MENU

	mi_tools = GTK_MENU_ITEM(gtk_menu_item_new_with_label( Text(MENU_DEVICES) ));
	gtk_menu_shell_append( menu, GTK_WIDGET(mi_tools) );

	item = gtk_menu_item_new_with_label( Text(MENU_TOOLS) );
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, item );

	update_tools_menu2( sub_menu );

#else

	mi_tools = GTK_MENU_ITEM(gtk_menu_item_new_with_label( Text(MENU_TOOLS) ));
	gtk_menu_shell_append( menu, GTK_WIDGET(mi_tools) );

#endif

	/* Test menu (Debug only) */

#if LOGLEVEL > 1
	item = gtk_menu_item_new_with_label( "Test" );
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, item );

	item = gtk_menu_item_new_with_label( "Status: Connected" );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(con_ic_status_connected), NULL );
	gtk_menu_shell_append( sub_menu, item );
	item = gtk_menu_item_new_with_label( "Status: Disconnecting" );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(con_ic_status_disconnecting), NULL );
	gtk_menu_shell_append( sub_menu, item );
	item = gtk_menu_item_new_with_label( "Status: Disconnected" );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(con_ic_status_disconnected), NULL );
	gtk_menu_shell_append( sub_menu, item );
#endif

#ifdef MAEMO

	/* Quit menu entry (Maemo only) */
	create_file_menu_items( menu );

#else

	item = gtk_menu_item_new_with_label( "" );
	gtk_widget_set_sensitive( item, FALSE );
	gtk_menu_item_set_right_justified( StatusMenuItem = GTK_MENU_ITEM(item), TRUE );
	gtk_menu_shell_append( menu, item );

	clock_timeout_id = g_timeout_add_full( G_PRIORITY_LOW, 10000, clock_timer, NULL, NULL );
	g_signal_connect( G_OBJECT(MainWindow), "window-state-event", G_CALLBACK(main_window_state_event), NULL );


#endif

	TRACE(( "<- create_menu()\n" ));
	return menu;
}

static GSList *append_tools_menu( GSList *group, GtkMenuShell *menu, const char *label, const upnp_device *sr )
{
	GtkWidget *item = gtk_radio_menu_item_new_with_label( group, label );
	group = gtk_radio_menu_item_get_group( GTK_RADIO_MENU_ITEM(item) );
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(item), sr == GetAVTransport() );
	g_object_set_data( G_OBJECT(item), upnp_device_key, (gpointer)sr );
	g_signal_connect( G_OBJECT(item), "toggled", G_CALLBACK(renderer_toggled), (gpointer)sr );
	gtk_menu_shell_append( menu, item );

	return group;
}

static void info( void /*GtkMenuItem* menu_item, gpointer user_data*/ )
{
	Info( NULL );
}

void update_view_menu( void )
{
	if ( Settings->startup.classic_ui )
		gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_view[GetCurrentPage()]), TRUE );

	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_fullscreen), GetFullscreen() );
}

void update_tools_menu( void )
{
	struct device_list_entry *sr_list;
	const upnp_device *device;
	GtkMenuShell *tools_menu;
	gboolean separator;
	GtkWidget *item;
	int i, n;

	TRACE(( "update_tools_menu(): delete old tools menu...\n" ));

	SetPlaylistRendererMenu( NULL );
	gtk_menu_item_set_submenu( mi_tools, NULL );

	TRACE(( "update_tools_menu(): build new tools menu...\n" ));

	tools_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( mi_tools, GTK_WIDGET(tools_menu) );

#if SHOW_DEVICE_MENU
	item = gtk_menu_item_new_with_label( Text(MENU_INFO) );
#else
	item = gtk_menu_item_new_with_label( Text(MENU_DEVICE_INFO) );
#endif
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(info), NULL );
	gtk_menu_shell_append( tools_menu, item );
	gtk_widget_set_sensitive( item, upnp_num_devices( NULL ) > 0 );

	separator = TRUE;

	sr_list = GetServerList( &n );
	ASSERT( sr_list != NULL );

	for ( i = 0; i < n; i++ )
	{
		GtkMenuShell *sub_menu;

		device = sr_list[i].device;
		ASSERT( device != NULL );
		if ( device == NULL ) break;

		/*if ( !is_twonky( device ) ) continue;*/

		if ( separator )
		{
			gtk_menu_shell_append( tools_menu, gtk_separator_menu_item_new() );
			separator = FALSE;
		}

		item = gtk_menu_item_new_with_label( device->friendlyName );
		sub_menu = GTK_MENU_SHELL(gtk_menu_new());
		gtk_menu_item_set_submenu( GTK_MENU_ITEM(item), GTK_WIDGET(sub_menu) );
		gtk_menu_shell_append( tools_menu, item );

		if ( is_twonky( device ) )
		{
			AppendTwonkyMenuItems( sub_menu, device );
			gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );
		}

		AppendDeviceMenuItems( sub_menu, device );
	}

	g_free( sr_list );

	device = GetAVTransport();
	if ( device != NULL && !upnp_device_is( device, upnp_serviceId_AVTransport ) )
	{
		gboolean more_than_one_linn_ds = FALSE;

		sr_list = GetRendererList( &n );
		ASSERT( sr_list != NULL );

	#if LOGLEVEL < 2
		for ( i = 0; i < n; i++ )
		{
			const upnp_device *sr = sr_list[i].device;
			ASSERT( sr != NULL );
			if ( sr == NULL ) break;

			if ( sr != device && !upnp_device_is( sr, upnp_serviceId_AVTransport ) )
			{
				more_than_one_linn_ds = TRUE;
				break;
			}
		}

		if ( more_than_one_linn_ds )
	#endif
		{
			GSList *group1 = NULL, *group2 = NULL;

			gtk_menu_shell_append( tools_menu, gtk_separator_menu_item_new() );
			m_renderer = GTK_MENU( gtk_menu_new() );

			for ( i = 0; i < n; i++ )
			{
				GString *label;

				device = sr_list[i].device;
				ASSERT( device != NULL );
				if ( device == NULL ) continue;

				if ( upnp_device_is( device, upnp_serviceId_AVTransport ) ) continue;

				label = g_string_new( device->friendlyName );
				if ( sr_list[i].product_id != 0 )
					g_string_append_printf( label, " (%lu)", sr_list[i].product_id );

				group1 = append_tools_menu( group1, tools_menu, label->str, device );
				group2 = append_tools_menu( group2, GTK_MENU_SHELL(m_renderer), label->str, device );

				g_string_free( label, TRUE );
			}

			SetPlaylistRendererMenu( m_renderer );
		}

		g_free( sr_list );
	}

#if !SHOW_DEVICE_MENU

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( tools_menu, item );

	update_tools_menu2( tools_menu );

#endif

	gtk_widget_show_all( GTK_WIDGET(tools_menu) );

	TRACE(( "update_tools_menu(): done.\n" ));
}

void AppendDeviceMenuItems( GtkMenuShell *menu, const upnp_device *device )
{
	GtkWidget *item;

	if ( device->presentationURL != NULL )
	{
		item = gtk_menu_item_new_with_label( Text(PRESENTATION_URL) );
		g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(ShowUri), (gpointer)device->presentationURL );
		gtk_menu_shell_append( menu, item );
	}

	if ( device->manufacturerURL != NULL )
	{
		item = gtk_menu_item_new_with_label( Text(MANUFACTURER_URL) );
		g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(ShowUri), (gpointer)device->manufacturerURL );
		gtk_menu_shell_append( menu, item );
	}
}

void AppendTwonkyMenuItems( GtkMenuShell *menu, const upnp_device *device )
{
	GtkWidget *item;

	ASSERT( is_twonky( device ) );

	item = gtk_menu_item_new_with_label( Text(SERVER_RESCAN) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(twonky_rescan), (gpointer)device );
	gtk_menu_shell_append( menu, item );

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( menu, item );

	item = gtk_menu_item_new_with_label( Text(SERVER_RESTART) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(twonky_restart), (gpointer)device );
	gtk_menu_shell_append( menu, item );

	item = gtk_menu_item_new_with_label( Text(SERVER_REBUILD) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(twonky_rebuild), (gpointer)device );
	gtk_menu_shell_append( menu, item );
}

void update_tools_menu2( GtkMenuShell *tools_menu )
{
	GtkWidget *item;

	TRACE(( "-> update_tools_menu2()\n" ));

	item = gtk_menu_item_new_with_label( Text(MENU_CUSTOMISE) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(Customise), NULL );
	gtk_menu_shell_append( tools_menu, item );

	item = gtk_menu_item_new_with_label( Text(MENU_OPTIONS) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(Options), NULL );
	gtk_menu_shell_append( tools_menu, item );

	item = gtk_separator_menu_item_new();
	gtk_menu_shell_append( tools_menu, item );

	item = gtk_menu_item_new_with_label( Text(MENU_ABOUT) );
	g_signal_connect( G_OBJECT(item), "activate", G_CALLBACK(about), NULL );
	gtk_menu_shell_append( tools_menu, item );

	TRACE(( "<- update_tools_menu2()\n" ));
}

/* Create the toolbar needed for the main view */
GtkToolbar *create_toolbar( void )
{
	GtkToolbar *toolbar = GTK_TOOLBAR( gtk_toolbar_new() );
	const guint8 *qgn_list_gene_music_file;
	gint icon_width, icon_index;

	TRACE(( "-> create_toolbar()\n" ));

	/* Set toolbar properties */
	gtk_toolbar_set_orientation( toolbar, GTK_ORIENTATION_HORIZONTAL );
	gtk_toolbar_set_style( toolbar, GTK_TOOLBAR_ICONS );
	/*gtk_toolbar_set_tooltips( toolbar, TRUE );*/

	icon_width = get_icon_width_for_toolbar( toolbar );
#ifdef MAEMO
	ASSERT( icon_width == 26 );
	icon_index = 0;
	qgn_list_gene_music_file = qgn_list_gene_music_file_26x26;
#else
	icon_index = 0;  /*( gtk_major_version == 2 && gtk_minor_version < 11 ) ? 0 : 2;*/
	if ( icon_width >= 24 ) icon_index++;
	qgn_list_gene_music_file = ( icon_index == 0 ) ? qgn_list_gene_music_file_16x16 : qgn_list_gene_music_file_24x24;
#endif

	/* Create & insert browser & playlist toolbar button items */
	CreateBrowserToolItems( toolbar );
	/*CreatePlaylistToolItems( toolbar );*/ /* moved to extra toolbar in v0.2 */
	CreateNowPlayingToolItems( toolbar, icon_index );

	if ( Settings->startup.classic_ui )
	{
		gtk_toolbar_insert( toolbar, gtk_separator_tool_item_new(), -1 );

		/* Create & insert notebook toolbar button items */
		ti_notebook[0] = gtk_radio_tool_button_new_from_stock(
			NULL, LEIADS_STOCK_BROWSER );
		fix_hildon_tool_button( ti_notebook[0] );
		gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_notebook[0]), Text(BROWSER) );
		gtk_tool_item_set_tooltip_text( ti_notebook[0], Text(BROWSER) );
		g_signal_connect( G_OBJECT(ti_notebook[0]), "toggled", G_CALLBACK(notebook_toggled), GINT_TO_POINTER(BROWSER_PAGE) );
		gtk_toolbar_insert( toolbar, ti_notebook[0], -1 );

		ti_notebook[1] = gtk_radio_tool_button_new_with_stock_from_widget(
			GTK_RADIO_TOOL_BUTTON(ti_notebook[0]), LEIADS_STOCK_PLAYLIST );
		fix_hildon_tool_button( ti_notebook[1] );
		gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_notebook[1]), Text(PLAYLIST) );
		gtk_tool_item_set_tooltip_text( ti_notebook[1], Text(PLAYLIST) );
		g_signal_connect( G_OBJECT(ti_notebook[1]), "toggled", G_CALLBACK(notebook_toggled), GINT_TO_POINTER(PLAYLIST_PAGE) );
		gtk_toolbar_insert( toolbar, ti_notebook[1], -1 );


		ti_notebook[2] = gtk_radio_tool_button_new_with_stock_from_widget(
			GTK_RADIO_TOOL_BUTTON(ti_notebook[1]),
			register_stock_icon( LEIADS_STOCK_NOW_PLAYING, qgn_list_gene_music_file ) );
		fix_hildon_tool_button( ti_notebook[2] );
		gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_notebook[2]), Text(NOW_PLAYING) );
		gtk_tool_item_set_tooltip_text( ti_notebook[2], Text(NOW_PLAYING) );
		g_signal_connect( G_OBJECT(ti_notebook[2]), "toggled", G_CALLBACK(notebook_toggled), GINT_TO_POINTER(NOW_PLAYING_PAGE) );
		gtk_toolbar_insert( toolbar, ti_notebook[2], -1 );
	}
	else
	{
		OnSelectBrowser();
		OnSelectPlaylist();
		OnSelectNowPlaying();
	}

	/* Create & insert media toolbar button items */
	CreateTransportToolItems( toolbar, icon_index );

	/* Create & insert volume toolbar button items */
	CreateVolumeToolItems( toolbar );

	TRACE(( "<- create_toolbar()\n" ));
	return toolbar;
}

/*-----------------------------------------------------------------------------------------------*/

struct search_data
{
	volatile gboolean search_server, search_renderer;
	struct animation *animation;
	guint message_id;
};

static void update_search_status( struct search_data *data )
{
#ifdef MAEMO

	;

#else

	const char *text;

	if ( data->message_id )
	{
		remove_from_statusbar( data->message_id );
		data->message_id = 0;
	}

	if ( data->search_server && data->search_renderer )
		text = Text(SEARCHING_SERVER_AND_RENDERER);
	else if ( data->search_server )
		text = Text(SEARCHING_SERVER);
	else if ( data->search_renderer )
		text = Text(SEARCHING_RENDERER);
	else
		text = NULL;

	if ( text != NULL )
	{
		gchar *s = g_strconcat( text, "...", NULL );
		data->message_id = push_to_statusbar( NULL, s );
		g_free( s );
	}

#endif
}

gboolean status_connected( gpointer user_data )
{
	gdk_threads_enter();

	if ( connected )
	{
		struct search_data *data;

		http_set_user_agent( "Leia DS/" VERSION );

	#ifdef CON_IC_CONNECTION
		clear_animation( connection_animation );
		connection_animation = NULL;
	#endif

		data = g_new( struct search_data, 1 );

		data->search_server = Settings->ssdp.media_server;
		data->search_renderer = TRUE;
		data->animation = show_animation(
			( data->search_server ) ? Text(SEARCHING_SERVER_AND_RENDERER) : Text(SEARCHING_RENDERER),
			NULL, 0 );
		data->message_id = 0;

		update_search_status( data );
		g_thread_create( search_thread, data, FALSE, NULL );
	}
	else
	{
		TRACE(( "*** status_connected(): Not connected\n" ));
	}

	gdk_threads_leave();
	return FALSE;
}

gboolean search_callback( struct _GTimer *timer, void *user_data )
{
	struct search_data *data = (struct search_data *)user_data;
	int num_server;

	if ( quitting ) return TRUE;  /* abort searching */

	num_server = upnp_num_devices( upnp_serviceId_ContentDirectory );

	if ( data->search_renderer )
	{
		int num_ds, num_preamp;
		upnp_device *device;

		NumServer = num_server;

		for ( device = upnp_get_first_device( upnp_serviceId_AVTransport ); device != NULL; device = upnp_get_next_device( device, upnp_serviceId_AVTransport ) )
		{
		#ifdef FILTER_LINN_UPNP_AV_RENDERER
			if ( device->manufacturer != NULL && strcmp( device->manufacturer, "Linn Products Ltd" ) == 0 )
			{
				upnp_device_mark_invalid( device );
				continue;
			}
		#endif

			if ( device->user_data == NULL )
			{
				upnp_string Sink;
				int success = ConnectionManager_GetProtocolInfo( device, NULL, &Sink, NULL );
				TRACE(( "search_callback(): ConnectionManager_GetProtocolInfo() = %s\n", (success) ? "TRUE" : "FALSE" ));
				if ( success )
				{
					TRACE(( "search_callback(): Sink = \"%s\"\n", Sink ));

					if ( strstr( Sink, "http-get:*:audio/" ) != NULL )
					{
						device->user_data = g_strdup( Sink );
						device->free_user_data = g_free;
					}
					else
					{
						upnp_device_mark_invalid( device );
					}

					upnp_free_string( Sink );
				}
			}
		}

		NumRenderer = upnp_num_av_transport();

		num_ds = num_preamp = 0;
		for ( device = upnp_get_first_device( NULL ); device != NULL; device = upnp_get_next_device( device, NULL ) )
		{
			if ( upnp_get_ds_compatibility_family( device ) != NULL )
				num_ds++;
			else if ( upnp_device_is( device, linn_serviceId_Preamp ) )
				num_preamp++;
		}

		TRACE(( "NumServer = %d, NumRenderer = %d, num_ds = %d, num_preamp = %d\n", NumServer, NumRenderer, num_ds, num_preamp ));
		if ( NumRenderer > 0 && num_ds >= num_preamp )
		{
			data->search_renderer = FALSE;
			g_idle_add( searching_1, data );
			if ( !Settings->ssdp.media_server ) return TRUE;  /* no need to continue searching */
		}

		return FALSE;  /* continue searching */
	}
	else if ( data->search_server )
	{
		if ( !Settings->ssdp.media_server )
		{
			/* Setting was just changed by user */
			data->search_server = FALSE;
			return TRUE;
		}

		if ( num_server != NumServer )
		{
			NumServer = num_server;

			TRACE(( "NumServer = %d\n", NumServer ));
			g_idle_add( searching_2, data );
		}

		return FALSE;  /* continue searching */
	}

	return TRUE;
}

gpointer search_thread( gpointer user_data )
{
	struct search_data *data = (struct search_data *)user_data;
	GError *error = NULL;
	const char *stv[4];
	int stc = 0;

	TRACE(( "search_thread(): upnp_search_devices()...\n" ));

	upnp_free_all_devices();

	SSDP_MX      = Settings->ssdp.mx;
	SSDP_TIMEOUT = Settings->ssdp.timeout;

	if ( Settings->ssdp.media_server )
	{
		stv[stc++] = upnp_device_MediaServer_1;
	}
	if ( Settings->ssdp.media_renderer )
	{
		stv[stc++] = upnp_device_MediaRenderer_1;
	}
	if ( Settings->ssdp.linn_products )
	{
		stv[stc++] = "urn:linn-co-uk:service:Product:1";       /* Auskerry, Bute, and Cara */
		stv[stc++] = "urn:av-openhome-org:service:Product:1";  /* Davaar */
	}

	upnp_search_devices( stv, stc, search_callback, user_data, &error );
	HandleError( error, Text(ERROR_UPNP_SEARCH) );

	g_idle_add( end_of_search, data );
	return NULL;
}

gboolean searching_1( gpointer user_data )
{
	struct search_data *data = (struct search_data *)user_data;
	const upnp_device *device;

	gdk_threads_enter();

	clear_animation( data->animation );
	data->animation = NULL;

	update_search_status( data );

	if ( NumRenderer > 1 || Settings->ssdp.default_media_renderer != NULL )
	{
		ASSERT( !BrowserIsOpen() );
		OpenBrowser();

		if ( (device = ChooseRenderer()) == NULL )
		{
			Quit();
		}
	}
	else
	{
		device = upnp_get_first_av_transport();
	}

	/*if ( device != NULL ) open_renderer( device );*/
	SetRenderer( device, FALSE );

	/* Since Cara 6 the content of the browser depends on the selected renderer,
	   so we try to open the browser as late as possible now */
	if ( !BrowserIsOpen() ) OpenBrowser();

	update_tools_menu();

	gdk_threads_leave();
	return FALSE;
}

gboolean searching_2( gpointer user_data )
{
	struct search_data *data = (struct search_data *)user_data;

	gdk_threads_enter();

	if ( UpdateBrowser() )
	{
		update_tools_menu();
	}
	else
	{
		data->search_server = FALSE;
		update_search_status( data );
	}

	gdk_threads_leave();
	return FALSE;
}

gboolean end_of_search( gpointer user_data )
{
	struct search_data *data = (struct search_data *)user_data;

	if ( data->search_renderer && NumRenderer )
	{
		data->search_renderer = FALSE;
		searching_1( user_data );
		gdk_threads_enter();
	}
	else
	{
		gdk_threads_enter();
		clear_animation( data->animation );
	}

	if ( !BrowserIsOpen() ) OpenBrowser();

	if ( data->search_renderer )
	{
		message_dialog( GTK_MESSAGE_ERROR,
			( data->search_server && NumServer == 0 )
				? Text(NO_SERVER_AND_RENDERER_FOUND) : Text(NO_RENDERER_FOUND) );
	}
	else if ( data->search_server && NumServer == 0 )
	{
		show_information( NULL, NULL, Text(NO_SERVER_FOUND) );
	}

	data->search_server = data->search_renderer = FALSE;
	update_search_status( data );

	g_free( data );

	gdk_threads_leave();

	return FALSE;
}

gboolean status_disconnected( gpointer data )
{
	gdk_threads_enter();

	if ( !connected )
	{
	#ifdef CON_IC_CONNECTION
		clear_animation( connection_animation );
		connection_animation = NULL;
	#endif

		close_all();
	}
	else
	{
		TRACE(( "*** status_disconnected(): Still connected\n" ));
	}

	gdk_threads_leave();
	return FALSE;
}

void close_all( void )
{
	busy_enter();

	upnp_unsubscribe_all( connected );
	http_stop_server();

	close_renderer( TRUE );
	CloseBrowser();
	NumServer = NumRenderer = 0;

	update_tools_menu();

	busy_leave();
}

/*-----------------------------------------------------------------------------------------------*/

gboolean open_renderer( const upnp_device *device )
{
	GError *error = NULL;
	gboolean result;

	ASSERT( device != NULL );

	SetAVTransport( device );

	result = PrepareForConnection( &error );
	TRACE(( "open_renderer(): PrepareForConnection() = %s\n", (result) ? "TRUE" : "FALSE" ));
	if ( result )
	{
		OpenPlaylist();
		OpenNowPlaying();
		OpenTransport();

		result = SubscribeMediaRenderer( renderer_callback, NULL, &error );
		TRACE(( "open_renderer(): SubscribeMediaRenderer() = %s\n", (result) ? "TRUE" : "FALSE" ));
		if ( result )
		{
			if ( OpenVolume() )
			{
				result = EnableVolumeEvents( &error );
				TRACE(( "open_renderer(): EnableVolumeEvents() = %s\n", (result) ? "TRUE" : "FALSE" ));
			}
		}
	}

	HandleError( error, Text(ERROR_CONNECT_DEVICE), device->friendlyName );
	return result;
}

void close_renderer( gboolean standby )
{
	int err;

	if ( connected )
	{
		err = UnsubscribeMediaRenderer( NULL );
		TRACE(( "UnsubscribeMediaRenderer() = %d\n", err ));

	}

	CloseVolume();
	CloseTransport();
	CloseNowPlaying();
	ClosePlaylist();

	if ( connected )
	{
		err = ConnectionComplete( standby, NULL );
		TRACE(( "ConnectionComplete() = %d\n", err ));
	}

	SetAVTransport( NULL );
}

static struct upnp_event
{
	struct upnp_event *next;

	const upnp_device *device;
	const char *serviceTypeOrId;
	upnp_ui4 SEQ;
	char *e_propertyset;
} *event_list;

gboolean renderer_callback( upnp_subscription *subscription, char *e_propertyset )
{
	struct upnp_event *e;

	TRACE(( "\n" ));
	TRACE(( "####### %s.%u \"%s\"\n", subscription->serviceTypeOrId, subscription->SEQ, e_propertyset ));

	e = g_new( struct upnp_event, 1 );
	ASSERT( e != NULL );

	e->device = subscription->device;
	e->serviceTypeOrId = subscription->serviceTypeOrId;
	e->SEQ = subscription->SEQ;
	e->e_propertyset = e_propertyset;

	list_append( &event_list, e );
	g_idle_add( renderer_callback_idle, NULL );

	return TRUE;
}

gboolean renderer_callback_idle( gpointer user_data )
{
	extern GMutex *gena_mutex;
	struct upnp_event *e;
	GError *error = NULL;

	for (;;)
	{
		g_mutex_lock( gena_mutex );
		e = event_list;
		event_list = (e != NULL) ? e->next : NULL;
		g_mutex_unlock( gena_mutex );

		if ( e == NULL ) break;

		gdk_threads_enter();
		busy_enter();
		HandleMediaRendererEvent( e->device, e->serviceTypeOrId, e->SEQ, e->e_propertyset, NULL, &error );
		HandleError( error, Text(ERROR_HANDLE_RENDERER_EVENT), e->device->friendlyName );
		busy_leave();
		gdk_threads_leave();

		upnp_free_string( e->e_propertyset );
		g_free( e );
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

void RegisterLeiaStockIcon( void )
{
	register_stock_icon( LEIADS_STOCK_LEIADS, leiads_icon_16x16 );
}

/*-----------------------------------------------------------------------------------------------*/

void ShowUri( GtkWidget *widget, const gchar *link )
{
	GError *error = NULL;
#if 1
#if defined( WIN32 )
	ShellExecute( NULL, "open", link, NULL, NULL, SW_SHOWNORMAL );
#elif defined( MAEMO )
	hildon_uri_open( link, HILDON_URI_ACTION_NORMAL, &error );
#else
	gchar *quoted_link = g_shell_quote( link );
	gchar *cmd;

	/* Note: "gnome-open" on a SmartQ device will try to launch "firefox" instead of "midori" */
	cmd = g_strconcat( IsSmartQ() ? "midori " : "gnome-open ", quoted_link, NULL );
	g_spawn_command_line_async( cmd, &error );

	g_free( cmd );
	g_free( quoted_link );
#endif
#else
	if ( widget == NULL ) widget = GTK_WIDGET(MainWindow);

	/* needs GTK 2.14 or newer */
	gtk_show_uri( gtk_widget_get_screen( widget ),
	              link, gtk_get_current_event_time(), &error );
#endif
	HandleError( error, Text(ERROR_OPEN_URL), link );
}

static void about_dialog_activate_url( GtkAboutDialog *about, const gchar *link, gpointer data )
{
	TRACE(( "about_dialog_activate_url( link = \"%s\", data = %p )\n", link, data ));
	ShowUri( GTK_WIDGET(about), link );
}

static void about_dialog_activate_email( GtkAboutDialog *about, const gchar *link, gpointer data )
{
	gchar *uri;

	TRACE(( "about_dialog_activate_email( link = \"%s\", data = %p )\n", link, data ));

	uri = g_strdup_printf( "mailto:%s", link );
	about_dialog_activate_url( about, uri, data );
	g_free( uri );
}

/* Callback for "About" menu entry */
void about( void )  /* (GtkMenuItem *menuitem, gpointer user_data) */
{
#ifdef MAEMO
	GdkPixbuf *logo = gdk_pixbuf_new_from_file( ICONS"/scalable/hildon/leiads.png", NULL );
#else
	GdkPixbuf *logo = gdk_pixbuf_new_from_inline( -1, leiads_icon_40x40, FALSE, NULL );
#endif

	gtk_about_dialog_set_url_hook( about_dialog_activate_url, NULL, NULL );
	gtk_about_dialog_set_email_hook( about_dialog_activate_email, NULL, NULL );

	About( logo );

	if ( logo != NULL ) g_object_unref( logo );
}

/*-----------------------------------------------------------------------------------------------*/

void debug_screenshot( void )
{
	take_screenshot( Settings->debug.screenshot * 1000, NULL );
}

void SetTransportState( enum TransportState new_state, gpointer user_data )
{
	if ( new_state != TransportState )
	{
		enum TransportState old_state = TransportState;
		TransportState = new_state;
		OnTransportState( old_state, new_state, user_data );
	}
}

/* Callback for "Close" menu entry */
gboolean Quit( void )
{
	TRACE(( "\n\n\n\n\n\n\n\n\n\nClosing application...\n" ));
	TRACE(( "http_connections = %d\n", http_connections ));

	if ( http_connections && !IsStopped() )
	{
		gint response = message_dialog( GTK_MESSAGE_QUESTION, Text(QUESTION_QUIT) );
		if ( response != GTK_RESPONSE_OK && response != GTK_RESPONSE_YES ) return TRUE;
		quitting = TRUE;
		DoStop();
	}

	quitting = TRUE;

	upnp_unsubscribe_all( connected );

	if ( IsOurLastfmSession( NULL ) )
	{
		TRACE(( "\n\nQuit(): Stopping Last.fm Radio...\n" ));
		if ( Stop( NULL ) ) PlaylistClear( NULL, NULL );
	}

	close_all();

	TRACE(( "num_xml_contents = %d\n", num_xml_contents() ));
	ASSERT( num_xml_contents() == 0 );

#ifndef MAEMO
	if ( hpaned != NULL && vpaned != NULL )
	{
		gint hposition = gtk_paned_get_position( hpaned );
		gint vposition = gtk_paned_get_position( vpaned );

		gchar *filename = BuildUserConfigFilename( leiads_ini );
		GKeyFile *key_file = g_key_file_new();
		gchar *data;

		g_key_file_set_integer( key_file, "Paned", "hposition", hposition );
		g_key_file_set_integer( key_file, "Paned", "vposition", vposition );

		data = g_key_file_to_data( key_file, NULL, NULL );
		if ( data != NULL )
		{
			g_file_set_contents( filename, data, -1, NULL );
		}

		g_key_file_free( key_file );
		g_free( filename );
	}
#endif

	gtk_main_quit();

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean GetFullscreen( void )
{
	GdkWindow *window = gtk_widget_get_window( GTK_WIDGET(MainWindow) );
	return (gdk_window_get_state( window ) & GDK_WINDOW_STATE_FULLSCREEN) != 0;
}

void SetFullscreen( gboolean active )
{
	gboolean is_active = GetFullscreen();

	TRACE(( "SetFullscreen( %s => %s )\n", (is_active) ? "TRUE" : "FALSE", (active) ? "TRUE" : "FALSE" ));

	if ( active )
	{
		if ( !is_active )
		{
			TRACE(( "SetFullscreen(): gtk_window_fullscreen()\n" ));
			set_fullscreen( TRUE );
		}
	}
	else
	{
		if ( is_active )
		{
			TRACE(( "SetFullscreen(): gtk_window_unfullscreen()\n" ));
			set_fullscreen( FALSE );
		}
	}
}

void set_fullscreen( gboolean active )
{
	TRACE(( "set_fullscreen( %s )\n", ( active ) ? "TRUE" : "FALSE" ));

	if ( active )
	{
		gtk_window_fullscreen( GTK_WINDOW(MainWindow) );
		gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_fullscreen), TRUE );
		SetNowPlayingAttributes( TRUE, -1 );
	}
	else
	{
		SetNowPlayingAttributes( FALSE, -1 );
		gtk_window_unfullscreen( GTK_WINDOW(MainWindow) );
		gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_fullscreen), FALSE );
	}
}

/*-----------------------------------------------------------------------------------------------*/

static void renderer_toggle_menu_callback( GtkWidget *widget, gpointer data )
{
	const upnp_device *sr = (const upnp_device *)g_object_get_data( G_OBJECT(widget), upnp_device_key );

	if ( sr == (const upnp_device *)data )
		gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(widget), TRUE );
}

static void renderer_toggle_menu( GtkMenu *menu, const upnp_device *device )
{
	TRACE(( "renderer_toggle_menu( menu = %p, device = %p )\n", (void *)menu, (void *)device ));

	if ( menu != NULL )
		gtk_container_foreach( GTK_CONTAINER(menu), renderer_toggle_menu_callback, (gpointer)device );
}

const upnp_device *GetRenderer( void )
{
	return GetAVTransport();
}

void SetRenderer( const upnp_device *device, gboolean refresh /*= FALSE*/ )
{
	TRACE(( "SetRenderer( %s ): %p => %p\n", ( refresh ) ? "TRUE" : "FALSE", (void *)GetRenderer(), (void *)device ));

	if ( device != GetRenderer() )
	{
		busy_enter();

		close_renderer( TRUE );
		if ( device != NULL ) open_renderer( device );

		renderer_toggle_menu( GTK_MENU( gtk_menu_item_get_submenu( mi_tools ) ), device );
		renderer_toggle_menu( m_renderer, device );

		OnBrowserSettingsChanged();  /* Add or remove "Radio Time" browser item */
		busy_leave();
	}
	else if ( refresh )
	{
		busy_enter();

		close_renderer( FALSE );
		if ( device != NULL ) open_renderer( device );

		busy_leave();
	}
}

/*-----------------------------------------------------------------------------------------------*/

int GetCurrentPage( void )
{
	return current_page_num;
}

void SetCurrentPage( int page_num )
{
	TRACE(( "SetCurrentPage( %d => %d )\n", current_page_num, page_num ));
	ASSERT( page_num >= 0 && page_num < NUM_PAGES );

	if ( Settings->startup.classic_ui && page_num >= 0 && page_num < NUM_PAGES && page_num != current_page_num )
	{
		gtk_notebook_set_current_page( notebook, page_num );

		gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_view[page_num]), TRUE );
		gtk_toggle_tool_button_set_active( GTK_TOGGLE_TOOL_BUTTON(ti_notebook[page_num]), TRUE );
	}
}

/*-----------------------------------------------------------------------------------------------*/

void fullscreen_toggled( GtkCheckMenuItem *check_menu_item, gpointer user_data )
{
	gboolean active = gtk_check_menu_item_get_active( check_menu_item );

	TRACE(( "# fullscreen_toggled( active = %s )\n", (active) ? "TRUE" : "FALSE" ));
	SetFullscreen( active );
}

void view_toggled( GtkCheckMenuItem *check_menu_item, gpointer user_data )
{
	gboolean active = gtk_check_menu_item_get_active( check_menu_item );
	int page_num = GPOINTER_TO_INT( user_data );

	TRACE(( "# view_toggled( active = %s, page_num = %d )\n", (active) ? "TRUE" : "FALSE", page_num ));
	if ( active ) SetCurrentPage( page_num );
}

void notebook_toggled( GtkToggleToolButton *toggle_tool_button, gpointer user_data )
{
	gboolean active = gtk_toggle_tool_button_get_active( toggle_tool_button );
	int page_num = GPOINTER_TO_INT( user_data );

	TRACE(( "# notebook_toggled( active = %s, page_num = %d )\n", (active) ? "TRUE" : "FALSE", page_num ));
	if ( active ) SetCurrentPage( page_num );
}

void page_switched( GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data )
{
	TRACE(( "# page_switched( page_num = %d => %d )\n", current_page_num, page_num ));

	switch ( current_page_num )
	{
	case BROWSER_PAGE:
		OnUnselectBrowser();
		break;
	case PLAYLIST_PAGE:
		OnUnselectPlaylist();
		break;
	case NOW_PLAYING_PAGE:
		OnUnselectNowPlaying();
		break;
	}

	current_page_num = page_num;

	switch ( current_page_num )
	{
	case BROWSER_PAGE:
		set_main_window_title( Text(BROWSER) );
		OnSelectBrowser();
		break;
	case PLAYLIST_PAGE:
		set_main_window_title( Text(PLAYLIST) );
		OnSelectPlaylist();
		break;
	case NOW_PLAYING_PAGE:
		set_main_window_title( Text(NOW_PLAYING) );
		OnSelectNowPlaying();
		break;
	}
}

void renderer_toggled( GtkMenuItem *menu_item, gpointer user_data )
{
	const upnp_device *device = (const upnp_device *)user_data;

	ASSERT( menu_item != NULL );

	if ( menu_item != NULL && gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(menu_item) ) )
	{
		TRACE(( "# renderer_toggled( %s => %s )\n",
			(GetAVTransport() != NULL) ? GetAVTransport()->friendlyName : "<NULL>",
			(device != NULL) ? device->friendlyName : "<NULL>" ));


		ASSERT( device != NULL );
		SetRenderer( device, TRUE );
	}
}

/*-----------------------------------------------------------------------------------------------*/

void con_ic_status_connected( void )
{
	if ( !connected )
	{
		connected = TRUE;
		g_idle_add( status_connected, NULL );
	}
}

void con_ic_status_disconnecting( void )
{
	upnp_unsubscribe_all( connected );
	con_ic_status_disconnected();
}

void con_ic_status_disconnected( void )
{
	if ( connected )
	{
		connected = FALSE;
		g_idle_add( status_disconnected, NULL );
	}
}

#ifdef CON_IC_CONNECTION
void connection_event( ConIcConnection *connection, ConIcConnectionEvent *event, gpointer user_data )
{
#ifdef CON_IC_CONNECTION
	cancel_animation( connection_animation );
#endif

	switch ( con_ic_connection_event_get_status( event ) )
	{
	case CON_IC_STATUS_CONNECTED:
		TRACE(( "CON_IC_STATUS_CONNECTED\n" ));
		con_ic_status_connected();
		break;
	case CON_IC_STATUS_DISCONNECTING:
		TRACE(( "CON_IC_STATUS_DISCONNECTING\n" ));
		con_ic_status_disconnecting();
		break;
	case CON_IC_STATUS_DISCONNECTED:
		TRACE(( "CON_IC_STATUS_DISCONNECTED\n" ));
		con_ic_status_disconnected();
		break;
	}
}
#endif

/*-----------------------------------------------------------------------------------------------*/

static int handle_error_flag;

static gboolean handle_error( gpointer data )
{
	gchar *error_message = (gchar *)data;

	gdk_threads_enter();
	message_dialog( GTK_MESSAGE_ERROR, error_message );
	gdk_threads_leave();

	g_atomic_int_set( &handle_error_flag, FALSE );

	g_free( error_message );
	return FALSE;
}

void HandleError( GError *error, const gchar *format, ... )
{
	ASSERT( format != NULL );
	if ( format == NULL ) return;

	if ( error != NULL )
	{
		gchar *message;
		va_list args;

		va_start( args, format );     /* Initialize variable arguments. */
		message = g_strdup_vprintf( format, args );
		va_end( args );               /* Reset variable arguments.      */

		TRACE(( "*** message = %s, error = %d:%s\n", message, error->code, (error->message != NULL) ? error->message : "<NULL>" ));
		TRACE(( "***\n" ));

		if ( g_atomic_int_compare_and_exchange( &handle_error_flag, FALSE, TRUE ) )
		{
			GString *error_message = g_string_new( "" );

			g_string_append_printf( error_message, Text(ERROR_MESSAGE), message );

			if ( error->message != NULL )
			{
				g_string_append( error_message, ":\n\n" );
				g_string_append( error_message, error->message );

				if ( strstr( error->message, "\n\n" ) == NULL )
					g_string_append( error_message, "\n" );
			}
			else
			{
				g_string_append( error_message, ".\n" );
			}

			if ( error->code != 0 )
			{
				g_string_append_printf( error_message, "\n%s = %d", Text(ERROR_CODE), error->code );
			}

			/*TRACE(( "*** error_message = %s\n", error_message->str ));*/
			g_idle_add_full( G_PRIORITY_LOW, handle_error, g_string_free( error_message, FALSE ), NULL );
		}

		g_free( message );
		g_error_free( error );
	}
}

/*-----------------------------------------------------------------------------------------------*/

gboolean is_twonky( const upnp_device *device )
{
	if ( device == NULL || device->manufacturer == NULL || device->modelName == NULL ) return FALSE;

	TRACE(( "device->manufacturer = %s\n", device->manufacturer ));
	TRACE(( "device->modelName = %s\n", device->modelName ));

	return ( strstr( device->friendlyName, "PVConnect" ) != NULL      /* TwonkyMedia in disguise */ ||
	         strcmp( device->manufacturer, "TwonkyVision GmbH" ) == 0 /* Twonky 4.x & 5.x        */ ||
	         strcmp( device->manufacturer, "PacketVideo" ) == 0       /* Twonky 6.x              */ ||
	         strcmp( device->modelName, "TwonkyMedia" ) == 0          /* Twonky 4.4              */ ||
	         strcmp( device->modelName, "TMS" ) == 0                  /* Twonky 5.1              */ );
}

static void twonky( const upnp_device *device, const char *url )
{
	GString *full_url = g_string_new( device->URLBase );
	GError *error = NULL;
	char *content;

	g_string_append( full_url, url );

	if ( http_get( full_url->str, 0, NULL, &content, NULL, &error ) )
	{
		gchar *info_text = xml_get_named_str( content, "body" );
		if ( info_text == NULL ) info_text = g_strdup( content );

		message_dialog( GTK_MESSAGE_INFO, info_text );
		g_free( info_text );

		ClearBrowserCache( device );
	}
	else
	{
		HandleError( error, Text(ERROR_CONNECT_DEVICE), device->friendlyName );
	}

	upnp_free_string( content );
	g_string_free( full_url, TRUE );
}

void twonky_rescan( GtkMenuItem *menu_item, const upnp_device *device )
{
	twonky( device, "rpc/rescan" );
}

void twonky_restart( GtkMenuItem *menu_item, const upnp_device *device )
{
	twonky( device, "rpc/restart" );
}

void twonky_rebuild( GtkMenuItem *menu_item, const upnp_device *device )
{
	GString *question = g_string_new( "" );
	gint response;

	g_string_printf( question, Text(SERVER_REBUILD_QUESTION), device->friendlyName );
#ifdef PLAYLIST_FILE_SUPPORT
	g_string_append( question, "\n\n" );
	g_string_append( question, Text(SERVER_REBUILD_QUESTION_X) );
#endif

	response = message_dialog( GTK_MESSAGE_QUESTION_WITH_CANCEL_AS_DEFAULT_BUTTON, question->str );
	g_string_free( question, TRUE );

	if ( response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES )
	{
		BrowserReset( device );
		twonky( device, "rpc/rebuild" );
	}
}

/*-----------------------------------------------------------------------------------------------*/

#ifndef MAEMO

static char last_time[16];

gboolean clock_timer( gpointer user_data )
{
	if ( show_clock && StatusMenuItem != NULL )
	{
		const char *format[4] = { "", "%X", "%I:%M %p", "%H:%M" };
		time_t t = time( NULL );
		char this_time[16];

		strftime( this_time, sizeof( this_time ) - 1, format[Settings->startup.clock_format], localtime( &t ) );
		this_time[sizeof( this_time ) - 1] = '\0';

		if ( Settings->startup.clock_format == 1 )
		{
		#ifdef WIN32

			SYSTEMTIME st;

			GetLocalTime( &st );
			GetTimeFormat( LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, this_time, sizeof( this_time ) );

		#else

			/* remove seconds */
			char *s = strchr( this_time, ':' );
			if ( s != NULL )
			{
				s = strchr( s + 1, ':' );
				if ( s != NULL ) strcpy( s, s + 3 );
			}

		#endif
		}

		/*TRACE(( "+++ TICK: %s -> %s +++\n", last_time, this_time ));*/
		if ( strcmp( last_time, this_time ) != 0 )
		{
			gchar *label = NULL;


			gdk_threads_enter();

			g_object_get( StatusMenuItem, "label", &label, NULL );
			if ( label != NULL && strcmp( last_time, label ) == 0 )
			{
				strcpy( last_time, this_time );

				TRACE(( "+++ CLOCK: %s +++\n", this_time ));
				g_object_set( StatusMenuItem, "label", this_time, NULL );
			}

			g_free( label );

			gdk_threads_leave();
		}
	}

	return TRUE;
}

gboolean main_window_state_event( GtkWidget *widget, GdkEventWindowState *event, gpointer user_data )
{
	TRACE(( "# main_window_state_event()\n" ));

	if ( (event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0 )
	{
		if ( (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0 )
		{
			g_object_set( StatusMenuItem, "label", last_time, NULL );
			/*gtk_widget_set_sensitive( GTK_WIDGET(StatusMenuItem), FALSE );*/
			show_clock = TRUE;
		}
		else
		{
			show_clock = FALSE;
			/*gtk_widget_set_sensitive( GTK_WIDGET(StatusMenuItem), FALSE );*/
			g_object_set( StatusMenuItem, "label", "", NULL );

			/* Workaround for SmartQ */
			if ( Settings->startup.maximize && (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) == 0 )
				gtk_window_maximize( GTK_WINDOW(MainWindow) );
		}
	}

	/* Workaround for bug on SmartQ devices: Do not allow the notebook to exceed the screen width */
	if ( notebook != NULL && (event->changed_mask & (GDK_WINDOW_STATE_MAXIMIZED|GDK_WINDOW_STATE_FULLSCREEN)) != 0 )
	{
		gint width = ( (event->new_window_state & (GDK_WINDOW_STATE_MAXIMIZED|GDK_WINDOW_STATE_FULLSCREEN)) != 0 )
				? Settings->screen_width : -1;
		gtk_widget_set_size_request( GTK_WIDGET(notebook), width, -1 );
	}

	return FALSE;
}

#endif

/*-----------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------*/
#ifndef MAEMO

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 12
#define GTK_TARGET_OTHER_APP     0
#define GTK_TARGET_OTHER_WIDGET  0
#endif

#define TARGET_ENTRY_INFO 0x4c656961 /* application assigned integer ID = 'Leia' */

GtkTargetEntry TargetList[2] =
{
	{ "STRING", GTK_TARGET_SAME_APP | GTK_TARGET_OTHER_WIDGET, TARGET_ENTRY_INFO },
	{ "text/uri-list", GTK_TARGET_OTHER_APP, TARGET_ENTRY_INFO }
};

void drag_data_received( GtkWidget *widget, GdkDragContext *drag_context, gint x, gint y, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data )
{
	GdkDragAction drag_action = drag_context->action;
	gboolean success = FALSE;
	gchar *didl;

	busy_enter();

	if ( drag_action == 0 || drag_action == GDK_ACTION_DEFAULT ) drag_action = (GdkDragAction)GPOINTER_TO_UINT( user_data );
	TRACE(( "# drag_data_received( drag_context->action = %d => %d )\n", drag_context->action, drag_action ));

	if ( selection_data != NULL && selection_data->length > 0 )
	{
		TRACE(( "  time                      = %d\n", time ));
		TRACE(( "  selection_data->selection = %s\n", gdk_atom_name(selection_data->selection) ));
		TRACE(( "  selection_data->target    = %s\n", gdk_atom_name(selection_data->target) ));
		TRACE(( "  selection_data->type      = %s\n", gdk_atom_name(selection_data->type) ));
		TRACE(( "  selection_data->format    = %d\n", selection_data->format ));
		TRACE(( "  selection_data->data      = %s\n", selection_data->data ));
		TRACE(( "  selection_data->length    = %d\n", selection_data->length ));

		if ( strcmp( gdk_atom_name(selection_data->target), TargetList[0].target ) == 0 )
		{
			didl = (gchar *)gtk_selection_data_get_text( selection_data );
			TRACE(( "  didl                      = %p\n", (void*)didl ));

			if ( didl != NULL )
			{
				char *endptr;
				long index = strtoul( didl, &endptr, 10 );

				TRACE(( "  didl                      = %s\n", didl ));

				if ( *endptr == '\0' )
				{
					g_free( didl );
					playlist_play_track( (int)index );
					success = TRUE;
				}
				else if ( is_dragable_item( didl, NULL ) )
				{
					if ( drag_action == GDK_ACTION_COPY )
					{
						success = browser_action( BROWSER_APPEND_TO_PLAYLIST, &didl, NULL, 0.0 );
					}
					else
					{
						success = browser_action( BROWSER_REPLACE_PLAYLIST, &didl, NULL, 0.0 );
					}
					/* didl will be freed by browser_action() */
				}
				else
				{
					g_free( didl );
				}
			}
		}
		else if ( strcmp( gdk_atom_name(selection_data->target), TargetList[1].target ) == 0 )
		{
			gchar **uris = g_uri_list_extract_uris( (const gchar *)selection_data->data );
			GError *error = NULL;
			int i;

			if ( uris != NULL && http_start_server( &error ) )
			{
				GString *playlist = g_string_new( "" );

				for ( i = 0; uris[i] != NULL; i++ )
				{
					GError *error = NULL;
					gchar *filename = g_filename_from_uri( uris[i], NULL, &error );
					struct stat s;

					TRACE(( "  uri[%d]                    = %s\n", i, (filename != NULL) ? filename : "<NULL>" ));

					if ( filename != NULL )
					{
						if ( g_stat( filename, &s ) == 0 )
						{
							if ( (s.st_mode & S_IFDIR) != 0 )
							{
								GError *error = NULL;
								GetMetadataFromFolder( filename, playlist, &error );
								HandleError( error, Text(ERROR_BROWSE_FOLDER), filename );
							}
							else if ( (s.st_mode & S_IFREG) != 0 )
							{
								GError *error = NULL;
								GetMetadataFromFile( filename, NULL, playlist, TRUE, &error );
								HandleError( error, Text(ERROR_GET_METADATA_FROM_FILE), filename );
							}
						}

						g_free( filename );
					}
					else
					{
						HandleError( error, Text(ERROR_OPEN_URL), uris[i] );
						break;
					}
				}

				g_strfreev( uris );

				didl = g_string_free( playlist, FALSE );
				if ( drag_action == GDK_ACTION_COPY )
				{
					success = browser_action( BROWSER_APPEND_TO_PLAYLIST, &didl, NULL, 0.0 );
				}
				else
				{
					success = browser_action( BROWSER_REPLACE_PLAYLIST, &didl, NULL, 0.0 );
				}
				/* didl will be freed by browser_action() */
			}
			else
			{
				HandleError( error, Text(ERROR_UPNP_STARTUP) );
			}
		}
	}

	gtk_drag_finish( drag_context, success, FALSE, time );

	busy_leave();
}

#endif
/*-----------------------------------------------------------------------------------------------*/
