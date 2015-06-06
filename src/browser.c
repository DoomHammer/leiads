/*
	browser.c
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

#include "../icons/26x26/qgn_list_gene_playlist.h"
#include "../icons/26x26/qgn_list_gene_music_file.h"

#define PIMP_MY_BROWSER                 /* Speeds up the browsing a tiny little bit */
#define BROWSER_BUSY_TIMEOUT      1000  /* Show busy animation after 1s */
#define BROWSER_CACHE_ENTRIES        5
#define BROWSER_MAX_LASTFM_NAME     50
#define PATH_SEPARATOR           '\001'
#define NUM_SPECIAL_FOLDERS          4  /* ".." resp. "Local Folder", "Internet Radio", "Radio Time", and "Last.fm Radio" */

const char mediaServerId[]       = GTK_STOCK_HARDDISK;
const char musicAlbumId[]        = GTK_STOCK_CDROM;
const char musicTrackId[]        = "qgn_list_gene_music_file";
const char audioBroadcastId[]    = GTK_STOCK_CONNECT;  /* TODO => audio broadcast icon */
const char audioItemId[]         = GTK_STOCK_FILE;
const char playlistContainerId[] = "qgn_list_gene_playlist";
const char containerId[]         = GTK_STOCK_DIRECTORY;

const char goBackId[]            = GTK_STOCK_GO_BACK;
const char localId[]             = GTK_STOCK_DIRECTORY;
const char localFolderId[]       = GTK_STOCK_DIRECTORY;  /* will be removed as soon as we support D&D of containerId */
const char radioId[]             = LEIADS_STOCK_LEIADS;
const char radiotimeId[]         = LEIADS_STOCK_RADIOTIME;
const char lastfmId[]            = LEIADS_STOCK_LASTFM;
const char newId[]               = GTK_STOCK_ADD;

static const char local_id[]     = "leiads:local";
static const char radio_id[]     = "leiads:radio";
static const char radiotime_id[] = "radiotime";
static const char lastfm_id[]    = "lastfm";

/*-----------------------------------------------------------------------------------------------*/

static GtkMenuItem *mi_browser;
static GtkWidget *mi_go_back, *mi_refresh, *mi_radio;
static GtkToolItem *ti_go_back;

static GtkScrolledWindow *scrolled_browser;
/*static*/ GtkTreeView *browser;   /* will be used by now_playing.c/set_current_radio_image() */
static GtkTreeViewColumn *browser_column;
static GtkCellRenderer *num_renderer;

/* TODO:
   Anstelle von nur id + title merken wir uns in Zukunft das komplette didl_item,
   damit wir get_upnp_sort_criteria() anwenden können.
*/
static GString *id_path = NULL;
static GString *title_path = NULL;
static GString *tree_path = NULL;
static GString *align_path = NULL;

#ifdef BROWSER_BUSY_TIMEOUT
static struct animation *browser_animation;
#endif

static upnp_device *browser_device;
static gchar *browser_sort_caps;

static gboolean browser_row_activated_by_key;
static int browser_row_activated_by_button;
static guint browser_row_activated_button;
static gint browser_row_activated_y;
static GtkTreePath *browser_row_activated_path;

static unsigned browse_id;

struct browse_data
{
	unsigned id;
	gchar *title;
	const char *sort_criteria;
	struct xml_content *result;
	GError *error;
};

#ifdef PIMP_MY_BROWSER

#ifdef _DEBUG
enum { COL_ICON = 0, COL_NUMBER, COL_TITLE, COL_CLASS, NUM_COLUMNS };
#else
enum { COL_ICON = 0, COL_NUMBER, COL_TITLE, NUM_COLUMNS };
#endif

static struct xml_content *browser_result;
static const char **browser_items;

#else

#ifdef _DEBUG
enum { COL_ITEM = 0, COL_ICON, COL_NUMBER, COL_TITLE, COL_CLASS, NUM_COLUMNS };
#else
enum { COL_ITEM = 0, COL_ICON, COL_NUMBER, COL_TITLE, NUM_COLUMNS };
#endif

#endif

/*-----------------------------------------------------------------------------------------------*/

GtkTreeView *create_browser( void );
void set_browser_device( upnp_device *device );

void start_browsing( gboolean use_thread );
struct xml_content *browse( const char *sort_criteria, GError **error );
struct xml_content *browse_children( const char *object_id, const char *sort_criteria, GError **error );
struct xml_content *browse_direct_children( const char *object_id, const char *sort_criteria, GError **error );

#ifdef BROWSER_BUSY_TIMEOUT
gpointer browse_thread( gpointer user_data );
gboolean browse_thread_end( gpointer user_data );
void close_browse_thread( struct browse_data *data );
#endif

void clear_browser( const char *title );
void set_browser_title( const char *title, int n );
void fill_browser( const char *title, struct xml_content *result );
void append_to_browser( GtkListStore *store, int i, const char *item, const char *class_id, const char *title, const char *number );

gboolean is_meta_title( const char *title );
const char *get_upnp_class_id( const char *didl_item );
const char *get_upnp_sort_criteria( const char *didl_item );
upnp_device *get_server( const char *UDN );
gboolean go_back( void );

void show_popup_menu( void );
void set_saved_item( gchar **didl_item, gchar **path, gfloat row_align );
void free_saved_item( void );

gboolean is_local_folder( const char *object_id );
const char *get_local_title( void );
struct xml_content *get_local_folder( const char *id, GError **error );
gchar *get_local_folder_path( const char *id );

gboolean is_radio_folder( const char *object_id );
const char *get_radio_title( void );
struct xml_content *get_radio_folder( GError **error );

gboolean is_radiotime_folder( const char *object_id );

gboolean is_lastfm_folder( const char *object_id );
void new_lastfm( void );
void add_lastfm( const char *didl_item );
gboolean play_lastfm( const char *didl_item );
gpointer play_lastfm_thread( gpointer user_data );
gpointer play_lastfm_thread_get( gpointer user_data );
gboolean play_lastfm_thread_end( gpointer user_data );

void update_browser_menu( void );
void browser_go_back( void );
void browser_refresh( void );
void browser_radio( void );

gboolean browser_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
gboolean browser_button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
gboolean browser_key_press_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data );
gboolean browser_key_release_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data );
void browser_selection_changed( GtkTreeSelection *selection, gpointer user_data );
void browser_row_activated( GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data );

gchar *get_browser_item( GtkTreeView *view, GtkTreePath *path, const gchar *path_str );
gboolean browser_action( int action, gchar **didl_item, gchar **path_str, gfloat row_align );
gboolean server_or_container_open( void /*GtkMenuItem *menu_item, gpointer user_data*/ );
gboolean server_info( void /*GtkMenuItem *menu_item, gpointer user_data*/ );
gboolean container_or_item_info( void /*GtkMenuItem *menu_item, gpointer user_data*/ );
gboolean container_or_item_replace( void /*GtkMenuItem *menu_item, gpointer user_data*/ );
gboolean container_or_item_append( void /*GtkMenuItem *menu_item, gpointer user_data*/ );
gboolean radio_set_channel( void /*GtkMenuItem *menu_item, gpointer user_data*/ );

gboolean ready_to_replace( void );

const char *get_current_path( GString *str );
void append_to_path( GString *path, const gchar *append );
void replace_current_path( GString *path, const gchar *replace );
gboolean go_back_in_path( GString *path );
gboolean can_go_back_in_path( void /*GString *path*/ );

/*-----------------------------------------------------------------------------------------------*/

const char *get_device_id( const char *object_id );

struct xml_content *get_from_browser_cache( const char *device_id, const char *object_id, const char *sort_criteria );
void remove_from_browser_cache( const char *device_id, const char *object_id );
void put_into_browser_cache( const char *device_id, const char *object_id, const char *sort_criteria, struct xml_content *result );
void clear_browser_cache( const char *device_id );

/*-----------------------------------------------------------------------------------------------*/
#ifndef MAEMO

void browser_drag_begin( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data );
void browser_drag_data_get( GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data );
/*gboolean browser_drag_failed( GtkWidget *widget, GdkDragContext *drag_context, GtkDragResult result, gpointer user_data );*/
void browser_drag_end( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data );



#endif
/*-----------------------------------------------------------------------------------------------*/

void CreateBrowserMenuItems( GtkMenuShell *menu )
{
	GtkMenuShell *sub_menu;
	gchar *label;

	TRACE(( "-> CreateBrowserMenuItems()\n" ));

	mi_browser = GTK_MENU_ITEM(gtk_menu_item_new_with_label( Text(BROWSER) ));
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( mi_browser, GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, GTK_WIDGET(mi_browser) );

	mi_go_back = gtk_menu_item_new_with_label( Text(BROWSER_GO_BACK) );
	g_signal_connect( G_OBJECT(mi_go_back), "activate", G_CALLBACK(browser_go_back), NULL );
	gtk_menu_shell_append( sub_menu, mi_go_back );

	gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );

	mi_refresh = gtk_menu_item_new_with_label( Text(KEYBD_REFRESH) );
	g_signal_connect( G_OBJECT(mi_refresh), "activate", G_CALLBACK(browser_refresh), NULL );
	gtk_menu_shell_append( sub_menu, mi_refresh );

	gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );

	label = g_strconcat( Text(RADIO), "...", NULL );
	mi_radio = gtk_menu_item_new_with_label( label );
	g_signal_connect( G_OBJECT(mi_radio), "activate", G_CALLBACK(browser_radio), NULL );
	gtk_menu_shell_append( sub_menu, mi_radio );
	g_free( label );

	TRACE(( "<- CreateBrowserMenuItems()\n" ));
}

void CreateBrowserToolItems( GtkToolbar *toolbar )
{
	TRACE(( "-> CreateBrowserToolItems()\n" ));

	ti_go_back = gtk_tool_button_new_from_stock( GTK_STOCK_GO_BACK );
	gtk_tool_item_set_tooltip_text( ti_go_back, Text(BROWSER_GO_BACK) );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_go_back), FALSE );
	g_signal_connect( G_OBJECT(ti_go_back), "clicked", G_CALLBACK(browser_go_back), NULL );
	gtk_toolbar_insert( toolbar, ti_go_back, -1 );

	TRACE(( "<- CreateBrowserToolItems()\n" ));
}

GtkWidget *CreateBrowser( void )
{
	/* Register stock icons (1) */
	RegisterLeiaStockIcon();
	RegisterRadioTimeStockIcon();
	RegisterLastfmStockIcon();

	/* Register stock icons (2) */
	register_stock_icon( playlistContainerId, qgn_list_gene_playlist_26x26 );
	register_stock_icon( musicTrackId, qgn_list_gene_music_file_26x26 );

	browser = create_browser();

	scrolled_browser = GTK_SCROLLED_WINDOW( new_scrolled_window() );
	if ( Settings->MID )
		gtk_scrolled_window_set_policy( scrolled_browser, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC );
	gtk_container_add( GTK_CONTAINER( scrolled_browser ), GTK_WIDGET( browser ) );

	return GTK_WIDGET( scrolled_browser );
}

int OpenBrowser( void )
{
	char dtostr_buf[G_ASCII_DTOSTR_BUF_SIZE];
	upnp_device *device;

	TRACE(( "-> OpenBrowser()\n" ));

	ASSERT( id_path == NULL );
	ASSERT( title_path == NULL );
	ASSERT( tree_path == NULL );
	ASSERT( align_path == NULL );

	device = upnp_get_first_device( upnp_serviceId_ContentDirectory );
	if ( device != NULL && upnp_get_next_device( device, upnp_serviceId_ContentDirectory ) == NULL )
	{
		set_browser_device( device );

		id_path = g_string_new( "0" );
		title_path = g_string_new( browser_device->friendlyName );
	}
	else
	{
		set_browser_device( NULL );

		id_path = g_string_new( "" );
		title_path = g_string_new( Text(BROWSER_MEDIA_SERVERS) );
	}

	tree_path = g_string_new( "-" );    /* no selection */
	align_path = g_string_new( g_ascii_dtostr( dtostr_buf, G_ASCII_DTOSTR_BUF_SIZE, 0.0 ) );

	gtk_widget_set_sensitive( GTK_WIDGET(mi_browser), TRUE );
	start_browsing( TRUE );

	TRACE(( "<- OpenBrowser()\n" ));
	return 0;
}

gboolean UpdateBrowser( void )
{
	upnp_device *device;

	TRACE(( "-> UpdateBrowser()\n" ));

	if ( can_go_back_in_path() ) return FALSE;  /* User has already entered a folder, abort */

	device = upnp_get_first_device( upnp_serviceId_ContentDirectory );
	if ( device != NULL && upnp_get_next_device( device, upnp_serviceId_ContentDirectory ) == NULL )
	{
		set_browser_device( device );

		g_string_assign( id_path, "0" );
		g_string_assign( title_path, browser_device->friendlyName );
	}
	else
	{
		set_browser_device( NULL );

		g_string_assign( id_path, "" );
		g_string_assign( title_path, Text(BROWSER_MEDIA_SERVERS) );
	}

	start_browsing( TRUE );

	TRACE(( "<- UpdateBrowser()\n" ));
	return TRUE;
}

void CloseBrowser( void )
{
	TRACE(( "-> CloseBrowser()\n" ));

	gtk_widget_set_sensitive( GTK_WIDGET(mi_browser), FALSE );

	if ( id_path    != NULL ) { g_string_free( id_path,    TRUE ); id_path    = NULL; }
	if ( title_path != NULL ) { g_string_free( title_path, TRUE ); title_path = NULL; }
	if ( tree_path  != NULL ) { g_string_free( tree_path,  TRUE ); tree_path  = NULL; }
	if ( align_path != NULL ) { g_string_free( align_path, TRUE ); align_path = NULL; }

	SetLastfmSessionKey( NULL, NULL );

	clear_browser_cache( NULL );
	clear_browser( NULL );

	set_browser_device( NULL );

	TRACE(( "<- CloseBrowser()\n" ));
}

gboolean BrowserIsOpen( void )
{
	return id_path != NULL;
}

void OnSelectBrowser( void )
{
	TRACE(( "-> OnSelectBrowser()\n" ));

	gtk_widget_set_sensitive( GTK_WIDGET(ti_go_back), can_go_back_in_path() );

	TRACE(( "<- OnSelectBrowser()\n" ));
}

void OnUnselectBrowser( void )
{
	TRACE(( "-> OnUnselectBrowser()\n" ));

	gtk_widget_set_sensitive( GTK_WIDGET(ti_go_back), FALSE );

	TRACE(( "<- OnUnselectBrowser()\n" ));
}

struct keybd_function browser_go_back_function =
	{ (void (*)( int delta, gboolean long_press ))browser_go_back, SUPPRESS_REPEAT };
struct keybd_function browser_refresh_function =
	{ (void (*)( int delta, gboolean long_press ))browser_refresh, SUPPRESS_REPEAT };

void OnBrowserKey( GdkEventKey *event, struct keybd_function **func )
{
	browser_row_activated_by_key = FALSE;
	
	switch ( event->keyval )
	{
	case GDK_BackSpace:
		*func = &browser_go_back_function;
		break;

	case GDK_Escape:
		if ( Settings->MID )  /* Nokia IT & SmartQ */
			*func = &browser_go_back_function;
		break;

	case GDK_Up:
	case GDK_Down:
	case GDK_Page_Up:
	case GDK_Page_Down:
		if ( Settings->keys.use_system_keys ) *func = NULL;
		break;

	case GDK_space:
	case GDK_KP_Space:
	case GDK_Return:
	case GDK_KP_Enter:
		if ( event->type == GDK_KEY_PRESS ) browser_row_activated_by_key = TRUE;
		if ( Settings->keys.use_system_keys ) *func = NULL;
		break;

	case GDK_F5:
		if ( BrowserIsOpen() && GTK_WIDGET_SENSITIVE( GTK_WIDGET(mi_refresh) ) )
			*func = &browser_refresh_function;
		break;
	}
}

void OnBrowserSettingsChanged( void )
{
	TRACE(( "OnBrowserSettingsChanged(): IsOpen = %s\n", BrowserIsOpen() ? "TRUE" : "FALSE" ));

	clear_browser_cache( local_id );
	clear_browser_cache( radiotime_id );

	if ( BrowserIsOpen() ) start_browsing( TRUE );
}

/*-----------------------------------------------------------------------------------------------*/

void BrowserReset( const upnp_device *device )
{
	TRACE(( "-> BrowserReset()\n" ));

	if ( device == browser_device )
	{
		while ( go_back() ) ;
		start_browsing( FALSE );
	}

	TRACE(( "<- BrowserReset()\n" ));
}

void ClearBrowserCache( const upnp_device *device )
{
	TRACE(( "-> ClearBrowserCache()\n" ));

	clear_browser_cache( device->UDN );

	TRACE(( "<- ClearBrowserCache()\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

GtkTreeView *create_browser( void )
{
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *store;

	GtkTreeView *browser = GTK_TREE_VIEW( gtk_tree_view_new() );
	gtk_tree_view_set_headers_visible( browser, TRUE );

	selection = gtk_tree_view_get_selection( browser );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_BROWSE );
	g_signal_connect( G_OBJECT(selection), "changed", GTK_SIGNAL_FUNC(browser_selection_changed), NULL );

	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "stock-id", COL_ICON, NULL );

	renderer = gtk_cell_renderer_text_new();
	g_object_set( renderer, "xalign", 1.0, NULL );
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_NUMBER, NULL );
	num_renderer = renderer;

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_TITLE, NULL );

	gtk_tree_view_append_column( browser, column );
	browser_column = column;

#ifdef _DEBUG
	column = gtk_tree_view_column_new();

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_CLASS, NULL );

	gtk_tree_view_append_column( browser, column );
#endif

	ASSERT( NUM_COLUMNS <= 5 );
	store = gtk_list_store_new( NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
	gtk_tree_view_set_model( browser, GTK_TREE_MODEL( store ) );
	g_object_unref( store );

	g_signal_connect( G_OBJECT(browser), "button-press-event", G_CALLBACK(browser_button_press_event), NULL );
	g_signal_connect( G_OBJECT(browser), "button-release-event", G_CALLBACK(browser_button_release_event), NULL );
	g_signal_connect( G_OBJECT(browser), "key-press-event", G_CALLBACK(browser_key_press_event), NULL );
	g_signal_connect( G_OBJECT(browser), "key-release-event", G_CALLBACK(browser_key_release_event), NULL );
	g_signal_connect( G_OBJECT(browser), "row-activated", GTK_SIGNAL_FUNC(browser_row_activated), NULL );

#ifndef MAEMO
	if ( !Settings->startup.classic_ui )
	{
		gtk_drag_source_set( GTK_WIDGET(browser), GDK_BUTTON1_MASK,
			TargetList, 1, GDK_ACTION_COPY|GDK_ACTION_MOVE );

		g_signal_connect( G_OBJECT(browser), "drag-begin", GTK_SIGNAL_FUNC(browser_drag_begin), NULL );
		g_signal_connect( G_OBJECT(browser), "drag-data-get", GTK_SIGNAL_FUNC(browser_drag_data_get), NULL );
		/*g_signal_connect( G_OBJECT(browser), "drag-failed", GTK_SIGNAL_FUNC(browser_drag_failed), NULL );*/
		g_signal_connect( G_OBJECT(browser), "drag-end", GTK_SIGNAL_FUNC(browser_drag_end), NULL );
	}
#endif

	return browser;
}

void set_browser_device( upnp_device *device )
{
	browser_device = device;

	g_free( browser_sort_caps );
	browser_sort_caps = NULL;

	if ( device != NULL )
	{
		upnp_string sort_caps = NULL;

		ContentDirectory_GetSortCapabilities( device, &sort_caps, NULL );
		if ( sort_caps != NULL )
		{
			browser_sort_caps = g_strdup( sort_caps );
			upnp_free_string( sort_caps );
		}
	}
}

void start_browsing( gboolean use_thread )
{
	const char *object_id = get_current_path( id_path );
	const char *title     = get_current_path( title_path );

	TRACE(( "-> start_browsing(): ObjectID = %s\n", (object_id != NULL) ? object_id : "<NULL>" ));

	busy_enter();

#ifdef BROWSER_BUSY_TIMEOUT
	close_browse_thread( NULL );
#endif
	clear_browser( title );

	if ( *object_id == '\0' )  /* browse server */
	{
		fill_browser( title, NULL );
	}
	else                /* browse children */
	{
		struct browse_data *data = g_new0( struct browse_data, 1 );

		data->id = ++browse_id;
		data->title = g_strdup( title );
		data->sort_criteria = NULL;  /* TODO */

	#ifdef BROWSER_BUSY_TIMEOUT
		if ( use_thread )
		{
			GError *error = NULL;

			browser_animation = show_animation( Text(BROWSER_OPENING), data->title, BROWSER_BUSY_TIMEOUT );
			gtk_widget_set_sensitive( GTK_WIDGET(mi_refresh), FALSE );

			g_thread_create( browse_thread, data, FALSE, &error );
			HandleError( error, Text(ERROR_BROWSE_FOLDER), data->title );
			return;
		}
		else
	#endif
		{
			data->result = browse( data->sort_criteria, &data->error );
			if ( data->result != NULL )
			{
				fill_browser( data->title, data->result );
				free_xml_content( data->result );
			}
		}

		HandleError( data->error, Text(ERROR_BROWSE_FOLDER), data->title );

		g_free( data->title );
		g_free( data );
	}

	update_browser_menu();
	busy_leave();

	TRACE(( "<- start_browsing()\n" ));
}

struct xml_content *browse( const char *sort_criteria, GError **error )
{
	const char *current_id = get_current_path( id_path );
	struct xml_content *result;

	TRACE(( "-> browse()\n" ));

	busy_enter();

	do
	{
		gchar *id = NULL;
		xml_iter iter;
		char *item;

		result = browse_children( current_id, sort_criteria, error );
		if ( result == NULL ) break;

		/* If there is only a "Music" directory... */
		for ( item = xml_first( result->str, &iter ); item != NULL; item = xml_next( &iter ) )
		{
			char title[16];
			const char **s;

			/* Directory? */
			if ( get_upnp_class_id( item ) != containerId )
			{
				xml_abort_iteration( &iter );
				break;
			}

			if ( xml_get_named( item, "dc:title", title, sizeof( title ) ) == NULL )
				continue;

			/* Ignore "- All -" etc. */
			if ( is_meta_title( title ) ) continue;

			/* "Music"? */
			for ( s = Music; *s != NULL; s++ )
			{
				if ( strcmp( title, *s ) == 0 ) break;
			}
			if ( *s != NULL && id == NULL )
			{
				id = xml_get_named_attribute_str( item, "id" );
				continue;
			}

			/* Ignore "Photos" */
			for ( s = Photos; *s != NULL; s++ )
			{
				if ( strcmp( title, *s ) == 0 ) break;
			}
			if ( *s != NULL ) continue;

			/* Ignore "Videos" */
			for ( s = Videos; *s != NULL; s++ )
			{
				if ( strcmp( title, *s ) == 0 ) break;
			}
			if ( *s != NULL ) continue;

			/* Something else! */
			xml_abort_iteration( &iter );
			break;
		}

		/* ...then we will enter it automatically */
		if ( id != NULL )
		{
			if ( item == NULL )
			{
				free_xml_content( result );
				result = NULL;

				TRACE(( "Replace current path: \"%s\" => \"%s\"...\n", get_current_path( id_path ), id ));
				replace_current_path( id_path, id );
				current_id = get_current_path( id_path );
			}

			g_free( id );
		}
	}
	while ( result == NULL );

	busy_leave();

	TRACE(( "<- browse() = %p\n", (void*)result ));
	return result;
}

struct xml_content *browse_children( const char *object_id, const char *sort_criteria, GError **error )
{
	struct xml_content *result;

	busy_enter();

	if ( is_radio_folder( object_id ) )
	{
		result = get_radio_folder( error );
	}
	else if ( is_lastfm_folder( object_id ) )
	{
		result = GetLastfmFolder( error );
	}
	else
	{
		const char *device_id = get_device_id( object_id );

		if ( browser_sort_caps == NULL || sort_criteria == NULL || strstr( browser_sort_caps, sort_criteria ) == NULL )
			sort_criteria = "";

		result = get_from_browser_cache( device_id, object_id, sort_criteria );
		if ( result == NULL )
		{
			result = browse_direct_children( object_id, sort_criteria, error );
			if ( result != NULL )
			{
				put_into_browser_cache( device_id, object_id, sort_criteria, result );
			}
		}
	}

	busy_leave();

	return result;
}

struct xml_content *browse_direct_children( const char *object_id, const char *sort_criteria, GError **error )
{
	upnp_ui4 starting_index = 0, number_returned, total_matches;
	struct xml_content *result = NULL;
	upnp_string didl_lite;

	ASSERT( !is_radio_folder( object_id ) && !is_lastfm_folder( object_id ) );

	if ( is_local_folder( object_id ) )
	{
		return http_start_server( error ) ? get_local_folder( object_id, error ) : NULL;
	}
	else if ( is_radiotime_folder( object_id ) )
	{
		return GetRadioTimeFolder( object_id, NULL /* formats: TODO */, NULL /* title: TODO */, error );
	}

	ASSERT( browser_device != NULL );

	for (;;)
	{
		struct xml_content *tmp;
		char *name, *content;

		if ( !ContentDirectory_Browse( browser_device, object_id,
				BrowseDirectChildren, "*", starting_index, 0, sort_criteria,
				&didl_lite, &number_returned, &total_matches, NULL, error ) )
		{
			TRACE(( "*** browse_direct_children(): ContentDirectory_Browse() failed\n" ));
			break;
		}

		TRACE(( "browse_direct_children( \"%s\" ): starting_index = %d, number_returned = %d, total_matches = %d\n",
			object_id, starting_index, number_returned, total_matches ));

		content = xml_unbox( &didl_lite, &name, NULL );
		if ( content == NULL || name == NULL || strcmp( name, "DIDL-Lite" ) != 0 )
		{
			TRACE(( "** browse_direct_children(): Unknown format\n" ));
			g_set_error( error, 0, 0, Text(ERRMSG_UNKNOWN_FORMAT) );

			upnp_free_string( didl_lite );
			break;
		}

		tmp = new_xml_content( upnp_shrink_to_fit( content ), number_returned, UPNP_FREE );
		result = append_xml_content( result, tmp, error );
		free_xml_content( tmp );

		if ( result == NULL )
		{
			TRACE(( "*** browse_direct_children(): append_to_browse_result() failed\n" ));
			break;
		}

		if ( number_returned <= 0 || result->n >= total_matches )
		{
			TRACE(( "browse_direct_children() = %d\n", result->n ));
			return result;
		}

		starting_index += number_returned;
	}

	free_xml_content( result );

	TRACE(( "browse_direct_children() = NULL\n" ));
	return NULL;
}

#ifdef BROWSER_BUSY_TIMEOUT

gpointer browse_thread( gpointer user_data )
{
	struct browse_data *data = (struct browse_data *)user_data;

	data->result = browse( data->sort_criteria, &data->error );

	cancel_animation( browser_animation );
	g_idle_add( browse_thread_end, data );
	return NULL;
}

gboolean browse_thread_end( gpointer user_data )
{
	struct browse_data *data = (struct browse_data *)user_data;

	gdk_threads_enter();

	if ( data->id == browse_id )
	{
		close_browse_thread( data );
	}
	else
	{
		free_xml_content( data->result );
		if ( data->error != NULL ) g_error_free( data->error );
	}

	gdk_threads_leave();

	g_free( data->title );
	g_free( data );
	return FALSE;
}

void close_browse_thread( struct browse_data *data )
{
	if ( browser_animation != NULL )
	{
		clear_animation( browser_animation );
		browser_animation = NULL;

		if ( data != NULL )
		{
			if ( data->result != NULL )
			{
				fill_browser( data->title, data->result );
				free_xml_content( data->result );
			}

			HandleError( data->error, Text(ERROR_BROWSE_FOLDER), data->title );
		}

		update_browser_menu();
		busy_leave();
	}
	else if ( data != NULL )
	{
		free_xml_content( data->result );
		if ( data->error != NULL ) g_error_free( data->error );
	}
}

#endif

void clear_browser( const char *title )
{
	GtkListStore *store = GTK_LIST_STORE( gtk_tree_view_get_model( browser ) );

	set_browser_title( title, -1 );
	gtk_list_store_clear( store );

#ifdef PIMP_MY_BROWSER
	g_free( (gpointer)browser_items ); browser_items = NULL;
	free_xml_content( browser_result ); browser_result = NULL;
#endif
}

void set_browser_title( const char *title, int n )
{
	GtkTreeViewColumn *column = browser_column;

	if ( title != NULL && n >= 0 )
	{
		GString *column_title = g_string_new( title );
		if ( n >= 0 ) g_string_append_printf( column_title, " [%d]", n );
		gtk_tree_view_column_set_title( column, column_title->str );
		g_string_free( column_title, TRUE );
	}
	else
	{
		if ( title == NULL ) title = "";
		gtk_tree_view_column_set_title( column, title );
	}
}

struct fill_browser_struct { GtkTreePath *path; gfloat row_align; };

static void browser_scroll_to_cell( GtkTreePath *path, gfloat row_align )
{
	TRACE(( "-> end_of_fill_browser()\n" ));

	ASSERT( tree_path != NULL );

	gtk_tree_view_scroll_to_cell( browser, path, NULL, TRUE, row_align, 0.0 );
	gtk_tree_path_free( path );

	TRACE(( "<- end_of_fill_browser()\n" ));
}

static gboolean browser_scroll_to_cell_idle( gpointer user_data )
{
	struct fill_browser_struct *fbs = (struct fill_browser_struct *)user_data;

	TRACE(( "-> _end_of_fill_browser()\n" ));

	gdk_threads_enter();
	browser_scroll_to_cell( fbs->path, fbs->row_align );
	gdk_threads_leave();

	g_free( fbs );

	TRACE(( "<- _end_of_fill_browser()\n" ));
	return FALSE;
}

void fill_browser( const char *title, struct xml_content *result )
{
	const gchar *current_path;
	gboolean show_number;
	GtkListStore *store;
	int count = 0, n, i;
	xml_iter iter;
	char *item;

	TRACE(( "-> fill_browser( result = %p )\n", (void *)result ));

	if ( !BrowserIsOpen() )  /* Browser is not open (anymore) */
	{
		TRACE(( "<- fill_browser(): *** browser is not open\n" ));
		return;
	}

	busy_enter();

	store = GTK_LIST_STORE( gtk_tree_view_get_model( browser ) );
	g_object_ref( store );
	gtk_tree_view_set_model( browser, NULL );

	if ( result == NULL )  /* browse server */
	{
		struct device_list_entry *sr_list = GetServerList( &n );
		ASSERT( sr_list != NULL );

	#ifdef PIMP_MY_BROWSER
		ASSERT( browser_items == NULL );
		browser_items = g_new0( const char *, n + NUM_SPECIAL_FOLDERS );
	#endif

		show_number = FALSE;

		for ( i = 0; i < n; i++ )
		{
			upnp_device *sr = sr_list[i].device;
			ASSERT( sr != NULL );
			if ( sr == NULL ) break;

			append_to_browser( store, i, sr->UDN, mediaServerId, sr->friendlyName, NULL );
			count++;
		}

		g_free( sr_list );
	}
	else                 /* browse children */
	{
		char numbers[100], number[12], tmp[16];
		gboolean show_artist = FALSE;

		size_t sizeof_buffer = 2 * 1024;
		char *buffer = g_new( char, sizeof_buffer );

		show_number = TRUE;

		for ( i = 0; i < sizeof( numbers ); i++ ) numbers[i] = 0;
		tmp[0] = '\0';

		for ( item = xml_first( result->str, &iter ); item != NULL; item = xml_next( &iter ) )
		{
			const char *class_id = get_upnp_class_id( item );

			if ( class_id == musicTrackId || class_id == audioBroadcastId || class_id == audioItemId )
			{
				if ( show_number && GetOriginalTrackNumber( item, number, sizeof( number ) ) != NULL )
				{
					i = atoi( number );
					if ( i > 0 && i < sizeof( numbers ) )
					{
						if ( GetTitle( item, buffer, sizeof( number ) ) != NULL && i == atoi( buffer ) )
							;  /* number is already part of the title */
						else if ( numbers[i]++ != 0 )
							show_number = FALSE;  /* number is ambiguous */
					}
				}
			}
			else if ( class_id != musicAlbumId )
			{
				continue;
			}

			if ( !show_artist && GetArtist( item, buffer, sizeof( tmp ) ) )
			{
				if ( tmp[0] != '\0' )
				{
					if ( strcmp( tmp, buffer ) != 0 )
						show_artist = TRUE;
				}
				else
				{
					strcpy( tmp, buffer );
				}
			}

			if ( !show_number && show_artist )
			{
				xml_abort_iteration( &iter );
				break;
			}
		}

		if ( show_number )
		{
			for ( i = 0; i < sizeof( numbers ); i++ )
			{
				if ( numbers[i] != 0 ) break;
			}
			if ( i == sizeof( numbers ) ) show_number = FALSE;
		}

		n = (int)result->n;
		i = 0;

	#ifdef PIMP_MY_BROWSER
		ASSERT( browser_result == NULL );
		browser_result = copy_xml_content( result );
		ASSERT( browser_items == NULL );
		browser_items = g_new0( const char *, n + NUM_SPECIAL_FOLDERS );
	#endif

		if ( Settings->browser.go_back_item && can_go_back_in_path() )
		{
			gchar *title = g_strdup_printf( "- %s -", Text(BROWSER_GO_BACK) );
			append_to_browser( store, i++, "..", goBackId, title, NULL );
			g_free( title );
		}

		for ( item = xml_first( result->str, &iter ); --n >= 0 && item != NULL; item = xml_next( &iter ) )
		{
			const char *class_id = get_upnp_class_id( item );

			/* Get track number */
			if ( show_number && GetOriginalTrackNumber( item, number, sizeof( number ) ) != NULL )
			{
				strcat( number, ". " );
			}
			else
			{
				number[0] = '\0';
			}

			GetTitle( item, buffer, sizeof_buffer );

			if ( !is_meta_title( buffer ) )
			{
				char *buf = buffer;
				int sizeof_buf = sizeof_buffer;
				gboolean show_info;

				while ( *buf != '\0' )
				{
					buf++;
					sizeof_buf--;
				}

				if ( show_artist )
				{
					if ( sizeof_buf > 3 && GetArtist( item, buf + 3, sizeof_buf - 3 ) != NULL )
					{
						const char *s = NULL, *t;
						for ( t = buffer; (t = strstr( t, buf + 3 )) != NULL; t++ ) s = t;

						if ( s != NULL && s - buffer >= 2 &&
						     ((s[-2] == '-' && s[-1] == ' ') ||
						      (s[-2] == ' ' && strchr( "([{", s[-1] ) != NULL)) )
						{
							;  /* The artist was already appended by the Media Server */
						}
						else
						{
							*buf++ = ' ';
							*buf++ = '-';
							*buf++ = ' ';
							sizeof_buf -= 3;

							while ( *buf != '\0' )
							{
								buf++;
								sizeof_buf--;
							}
						}
					}
				}

				if ( class_id == musicAlbumId )
				{
					show_info = ( sizeof_buf > 4+3 && GetYear( item, buf + 2, sizeof_buf - 3 ) != NULL );
				}
				else if ( class_id == musicTrackId || class_id == audioItemId )
				{
					show_info = ( sizeof_buf > 8+3 && GetDuration( item, buf + 2, sizeof_buf - 3 ) != NULL );
				}
				else if ( class_id == audioBroadcastId )
				{
					show_info = ( sizeof_buf > 3 && GetGenre( item, buf + 2, sizeof_buf - 3 ) != NULL );
				}
				else
				{
					show_info = FALSE;
				}

				if ( show_info )
				{
					*buf++ = ' ';
					*buf++ = '(';
					strcat( buf, ")" );
				}

				count++;
			}

			append_to_browser( store, i++, item, class_id, buffer, number );
		}

		/*ASSERT( n < 0 && item == NULL );*/
	#if LOGLEVEL > 0
		if ( n >= 0 || item != NULL )
		{
			GtkWidget *chooser;
			GtkFileFilter *filter;

			gchar *message = g_strdup_printf(
				"Either the Media Server has reported a wrong number of items or Leia has serious problems parsing the directory content.\n\n"
				"To examine this problem please save the directory content to a file right now and send it to axel.sommerfeldt@f-m.fm afterwards. "
				"This would be very kind.\n\n(Errorcode = %d)", n );
			message_dialog( GTK_MESSAGE_ERROR, message );
			g_free( message );

			if ( item != NULL ) xml_abort_iteration( &iter );

			chooser = new_file_chooser_dialog( "Save directory content", NULL, GTK_FILE_CHOOSER_ACTION_SAVE );

			filter = gtk_file_filter_new();
			gtk_file_filter_set_name( filter, "XML Files (*.xml;*.XML)" );
			gtk_file_filter_add_pattern( filter, "*.xml;*.XML" );
			gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(chooser), filter );

			gtk_file_chooser_set_filter( GTK_FILE_CHOOSER(chooser), filter );
			gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(chooser), "content.xml" );
			gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER(chooser), TRUE );

			if ( gtk_dialog_run( GTK_DIALOG(chooser) ) == GTK_RESPONSE_OK )
			{
				gchar *filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(chooser) );
				GError *error = NULL;

				g_file_set_contents( filename, result->str, -1, &error );
				HandleError( error, Text(ERROR_PLAYLIST_SAVE), filename );

				g_free( filename );
			}

			gtk_widget_destroy( chooser );
		}
	#endif

		g_free( buffer );
	}

	if ( !can_go_back_in_path() )
	{
		const char *title;

		title = get_local_title();
		if ( title != NULL )
			append_to_browser( store, i++, local_id, localId, title, NULL );

		title = get_radio_title();
		if ( title != NULL )
			append_to_browser( store, i++, radio_id, radioId, title, NULL );

		title = GetRadioTimeTitle();
		if ( title != NULL )
			append_to_browser( store, i++, radiotime_id, radiotimeId, title, NULL );

		title = GetLastfmRadioTitle();
		if ( title != NULL )
			append_to_browser( store, i++, lastfm_id, lastfmId, title, NULL );

		ASSERT( i <= ((result != NULL) ? (int)result->n : n) + NUM_SPECIAL_FOLDERS );
	}

#ifndef _DEBUG
	gtk_tree_view_column_set_sizing( browser_column,
		show_number ? GTK_TREE_VIEW_COLUMN_GROW_ONLY : GTK_TREE_VIEW_COLUMN_FIXED );
#endif
	gtk_tree_view_set_fixed_height_mode( browser, !show_number );
	gtk_cell_renderer_set_visible( num_renderer, show_number );

	set_browser_title( title, count );
	gtk_tree_view_set_model( browser, GTK_TREE_MODEL( store ) );
	g_object_unref( store );

	gtk_tree_view_set_enable_search( browser, TRUE );
	gtk_tree_view_set_search_column( browser, COL_TITLE );

	current_path = get_current_path( tree_path );
	if ( current_path != NULL && *current_path != '-' )
	{
		GtkTreePath *path = gtk_tree_path_new_from_string( current_path );
		gfloat row_align = (gfloat)g_ascii_strtod( get_current_path( align_path ), NULL );

		gtk_tree_view_set_cursor( browser, path, NULL, FALSE );

		if ( gtk_major_version == 2 && gtk_minor_version < 11 )
		{
			/* Workaround for bug in gtk_tree_view_scroll_to_cell() */
			struct fill_browser_struct *fbs = g_new( struct fill_browser_struct, 1 );
			fbs->path = path;
			fbs->row_align = row_align;
			g_idle_add( browser_scroll_to_cell_idle, fbs );
		}
		else
		{
			browser_scroll_to_cell( path, row_align );
		}
	}

	busy_leave();

	TRACE(( "<- fill_browser()\n" ));
}

void append_to_browser( GtkListStore *store, int i, const char *item, const char *class_id, const char *title, const char *number )
{
	GtkTreeIter iter;

	/*TRACE(( "browser_items[%d] := %s\n", i, item ));*/
#ifdef PIMP_MY_BROWSER
	browser_items[i] = item;
#endif

	gtk_list_store_insert_with_values( store, &iter, i,
	#ifndef PIMP_MY_BROWSER
		COL_ITEM, item,
	#endif
		COL_ICON, class_id,
		COL_TITLE, title,
		-1 );

	if ( number != NULL && number[0] != '\0' )
		gtk_list_store_set( store, &iter, COL_NUMBER, number, -1 );

#ifdef _DEBUG
{
	char value[64];
	if ( xml_get_named( item, "upnp:class", value, sizeof( value ) ) != NULL )
		gtk_list_store_set( store, &iter, COL_CLASS, value, -1 );
}
#endif
}

gboolean is_meta_title( const char *title )
{
	char ch = *title++;
	if ( ch == '-' || ch == '[' )
	{
		while ( *title++ != '\0' ) ;
		if ( ch == '-' && title[-2] == '-' ) return TRUE;
		if ( ch == '[' && title[-2] == ']' ) return TRUE;
	}

	return FALSE;
}

const char *get_upnp_class_id( const char *didl_item )
{
	const char *stock_id = NULL;
	char value[40];

	if ( xml_get_named( didl_item, "upnp:class", value, sizeof( value ) ) != NULL )
	{
		/*TRACE(( "get_upnp_class_id(): upnp:class = %s\n", value ));*/

		/* object.item.audioItem */
		if ( strcmp( value, "object.item.audioItem.musicTrack" ) == 0 )
			stock_id = musicTrackId;
		else if ( strcmp( value, "object.item.audioItem.audioBroadcast" ) == 0 )
			stock_id = audioBroadcastId;
		else if ( strncmp( value, "object.item.audioItem", 21 ) == 0 )
			stock_id = audioItemId;

		/* object.container */
		else if ( strcmp( value, "object.container.album.musicAlbum" ) == 0 )
		{
			stock_id = musicAlbumId;
		}
		/*else if ( strncmp( value, "object.container.album", 22 ) == 0 )
		{
			stock_id = albumId;
		}*/
		else if ( strcmp( value, "object.container.localFolder" ) == 0 )
		{
			stock_id = localFolderId;
		}
		else if ( strcmp( value, "object.container.playlistContainer" ) == 0 &&
		          xml_get_named( didl_item, "dc:title", value, sizeof( value ) ) != NULL && value[0] != '-' )
		{
			stock_id = playlistContainerId;
		}
		else if ( strcmp( value, "object.container.storageFolder" ) == 0 )
		{
			stock_id = containerId;

			/* Workaround for behaviour of Twonky 4.4.x */
			if ( Settings->browser.handle_storage_folder_with_artist_as_album &&
			     xml_get_named( didl_item, "upnp:artist", value, sizeof( value ) ) != NULL )
			{
				stock_id = musicAlbumId;
			}

			/* Workaround for behaviour of Twonky 5.1.x */
			else if ( Settings->browser.handle_storage_folder_with_genre_as_album &&
			          xml_get_named( didl_item, "upnp:genre", value, sizeof( value ) ) != NULL )
			{
				/* Workaround for bug in Twonky 5.1, otherwise "Folder" will be shown as album */
				if ( xml_get_named_attribute( didl_item, "parentID", value, sizeof( value ) ) != NULL &&
				     strcmp( value, "0$1" ) != 0 )
				{
					stock_id = musicAlbumId;
				}
			}
		}
		else if ( strncmp( value, "object.container", 16 ) == 0 )
		{
			stock_id = containerId;
		}

		else if ( strncmp( value, "object.new", 10 ) == 0 )
		{
			stock_id = newId;
		}
	}
	else if ( didl_item != NULL )
	{
		if ( strncmp( didl_item, "uuid:", 5 ) == 0 )
		{
			stock_id = mediaServerId;
		}
		else if ( strcmp( didl_item, local_id ) == 0 )
		{
			stock_id = localId;
		}
		else if ( strcmp( didl_item, radio_id ) == 0 )
		{
			stock_id = radioId;
		}
		else if ( strcmp( didl_item, radiotime_id ) == 0 )
		{
			stock_id = radiotimeId;
		}
		else if ( strcmp( didl_item, lastfm_id ) == 0 )
		{
			stock_id = lastfmId;
		}
		else if ( strcmp( didl_item, ".." ) == 0 )
		{
			stock_id = goBackId;
		}
	}

	return stock_id;
}

const char *get_upnp_sort_criteria( const char *didl_item )
{
	const char *class_id = get_upnp_class_id( didl_item );
	const char *sort_criteria = NULL;

	if ( class_id == musicAlbumId ) sort_criteria = "upnp:originalTrackNumber";

	return sort_criteria;
}

upnp_device *get_server( const char *UDN )
{
	upnp_device *device;

	for ( device = upnp_get_first_device( upnp_serviceId_ContentDirectory ); device != NULL; device = upnp_get_next_device( device, upnp_serviceId_ContentDirectory ) )
	{
		if ( device->UDN != NULL && strcmp( device->UDN, UDN ) == 0 )
		{
			return device;
		}
	}

	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

struct { gchar *didl; gchar *path; gfloat row_align; GtkMenu *menu; } saved_item;

static void menu_position_func( GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data )
{
	GtkTreePath *path = gtk_tree_path_new_from_string( saved_item.path );
	GtkTreeViewColumn *column = browser_column;
	GdkRectangle rect;

	TRACE(( "## menu_position_func()\n" ));

	/* Get origin of the tree view */
	gdk_window_get_origin( gtk_widget_get_window( GTK_WIDGET(browser) ), x, y );
	TRACE(( "(x,y): %d,%d\n", *x, *y ));

	/* Get the relative position of the selected row
	   Note: This is relative to the bottom of the tree view header */
	gtk_tree_view_get_cell_area( browser, path, column, &rect );
	gtk_tree_path_free( path );
	TRACE(( "rect: %d,%d,%d,%d\n", rect.x, rect.y, rect.width, rect.height ));

	/* Add height of the tree view header to make the rectange
	   relative to the upper left corner of the tree view window;
	   we simply assume it's as tall as the cell height */
	rect.y += rect.height;

	/* Add the relative position to the origin of the tree view,
	   plus some extra offset */
	*x += rect.x + rect.height * 4;
	*y += rect.y + rect.height - 1;
	TRACE(( "=> (x,y): %d,%d\n", *x, *y ));

	if ( *y + GTK_WIDGET(menu)->requisition.height >= Settings->screen_height )
	{
		*y -= rect.height - 1;
		*y -= GTK_WIDGET(menu)->requisition.height;
		*y += 3;
	}

	*push_in = FALSE;
}

void show_popup_menu( void )
{
	const char *class_id = get_upnp_class_id( saved_item.didl );
	GtkMenuShell *menu = GTK_MENU_SHELL( gtk_menu_new() );
	int num_menu_items = 0;
	GtkWidget *item;
	char value[16];

	TRACE(( "-> show_popup_menu(): browser_row_activated_by_key = %s\n", (browser_row_activated_by_key) ? "TRUE" : "FALSE" ));

	if ( class_id == mediaServerId || class_id == containerId ||
	     class_id == playlistContainerId || class_id == musicAlbumId ||
	     class_id == localId || class_id == radioId || class_id == radiotimeId || class_id == lastfmId )
	{
		item = gtk_menu_item_new_with_label( Text(BROWSER_OPEN) );
		g_signal_connect( item, "activate", G_CALLBACK(server_or_container_open), saved_item.didl );
		gtk_menu_shell_append( menu, item );
		num_menu_items++;
	}

	if ( class_id == mediaServerId )
	{
		const upnp_device *device = get_server( saved_item.didl );
		if ( device != NULL )
		{
			if ( is_twonky( device ) )
			{
				item = gtk_menu_item_new_with_label( Text(MENU_INFO) );
				g_signal_connect( item, "activate", G_CALLBACK(server_info), NULL );
				gtk_menu_shell_append( menu, item );
				num_menu_items++;

				gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );
				AppendTwonkyMenuItems( menu, device );
			}

			if ( device->presentationURL != NULL || device->manufacturerURL != NULL )
			{
				gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );
				AppendDeviceMenuItems( menu, device );
			}
		}
	}
	else if ( class_id == localId || class_id == radiotimeId || class_id == lastfmId )
	{
		gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );
		item = gtk_menu_item_new_with_label( Text(MENU_OPTIONS) );
		g_signal_connect( item, "activate", G_CALLBACK(Options), (gpointer)class_id );
		gtk_menu_shell_append( menu, item );
	}
	else if ( class_id == radioId )
	{
		GtkStockItem stock_item;

		gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );
		item = gtk_menu_item_new_with_mnemonic( gtk_stock_lookup( GTK_STOCK_EDIT, &stock_item ) ? stock_item.label : Text(MENU_OPTIONS) );
		g_signal_connect( item, "activate", G_CALLBACK(browser_radio), (gpointer)class_id );
		gtk_menu_shell_append( menu, item );
	}
	else if ( is_dragable_item( saved_item.didl, class_id ) )
	{
		if ( class_id == musicAlbumId || class_id == musicTrackId || class_id == audioItemId )
		{
			if ( IsValuableInfo( xml_get_named( saved_item.didl, "upnp:albumArtURI", value, sizeof( value ) ) ) ||
			     IsValuableInfo( xml_get_named( saved_item.didl, "upnp:artist", value, sizeof( value ) ) ) )
			{
				item = gtk_menu_item_new_with_label( Text(MENU_INFO) );
				g_signal_connect( item, "activate", G_CALLBACK(container_or_item_info), saved_item.didl );
				gtk_menu_shell_append( menu, item );
				num_menu_items++;
			}
		}
		else if ( class_id == audioBroadcastId )
		{
			if ( IsValuableInfo( xml_get_named( saved_item.didl, "upnp:albumArtURI", value, sizeof( value ) ) ) ||
			     IsValuableInfo( xml_get_named( saved_item.didl, "radiotime:subtext", value, sizeof( value ) ) ) )
			{
				item = gtk_menu_item_new_with_label( Text(MENU_INFO) );
				g_signal_connect( item, "activate", G_CALLBACK(container_or_item_info), saved_item.didl );
				gtk_menu_shell_append( menu, item );
				num_menu_items++;
			}
		}

		if ( num_menu_items > 0 ) gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );

		/*if ( Settings->browser.page_after_replace_playlist != 0 || GetTrackCount() > 0 )*/
		{
			item = gtk_menu_item_new_with_label( Text(BROWSER_REPLACE_PLAYLIST) );
			gtk_widget_set_sensitive( GTK_WIDGET(item), PlaylistIsOpen() );
			g_signal_connect( item, "activate", G_CALLBACK(container_or_item_replace), saved_item.didl );
			gtk_menu_shell_append( menu, item );
			num_menu_items++;
		}

		if ( !IsLastfmItem( saved_item.didl ) )
		{
			if ( !IsLastfmTrack )
			{
				item = gtk_menu_item_new_with_label( Text(BROWSER_APPEND_TO_PLAYLIST) );
				gtk_widget_set_sensitive( GTK_WIDGET(item), PlaylistIsOpen() );
				g_signal_connect( item, "activate", G_CALLBACK(container_or_item_append), saved_item.didl );
				gtk_menu_shell_append( menu, item );
				num_menu_items++;
			}
		}
	}

	gtk_menu_attach_to_widget( GTK_MENU(menu), GTK_WIDGET(browser), NULL );
	saved_item.menu = GTK_MENU(menu);

	if ( num_menu_items > 0 )
	{
		gtk_widget_show_all( GTK_WIDGET(menu) );
		gtk_menu_popup( GTK_MENU(menu), NULL, NULL,
			(browser_row_activated_by_key) ? menu_position_func : NULL, NULL,
			0, gtk_get_current_event_time() );
	}
	else
	{
		free_saved_item();
	}

	TRACE(( "<- show_popup_menu()\n" ));
}

gboolean is_dragable_item( const char *didl_item, const char *class_id )
{
	gboolean result = TRUE;

	if ( class_id == NULL ) class_id = get_upnp_class_id( didl_item );

	TRACE(( "is_dragable_item( %s )\n", class_id ));

	if ( /*class_id == containerId ||*/ class_id == playlistContainerId || class_id == musicAlbumId ||
	       class_id == musicTrackId || class_id == audioBroadcastId || class_id == audioItemId )
	{
		; /* ok, nothing to do */
	}
	else if ( class_id == localFolderId )
	{
		gchar *id = xml_get_named_attribute_str( didl_item, "id" );
		gchar *path = get_local_folder_path( id );
		GDir *dir;

		dir = g_dir_open( path, 0, NULL );
		if ( dir != NULL )
		{
			const char *name;

			while ( result && (name = g_dir_read_name( dir )) != NULL )
			{
				if ( name[0] != '.' )
				{
					gchar *filename = g_build_filename( path, name, NULL );
					struct stat s;

					if ( g_stat( filename, &s ) == 0 )
					{
						if ( (s.st_mode & S_IFDIR) != 0 )
						{
							TRACE(( "is_dragable_item(): %s is a directory\n", name ));
							result = FALSE;
						}
						else
						{
							TRACE(( "is_dragable_item(): %s is not a directory\n", name ));
						}
					}
					else
					{
						TRACE(( "*** is_dragable_item(): g_stat( %s ) failed\n", filename ));
					}

					g_free( filename );
				}
			}

			g_dir_close( dir );
		}

		g_free( path );
		g_free( id );
	}
	else
	{
		result = FALSE;
	}

	return result;
}

void set_saved_item( gchar **didl_item, gchar **path, gfloat row_align )
{
	free_saved_item();

	ASSERT( didl_item != NULL );
	saved_item.didl = *didl_item; *didl_item = NULL;
	if ( path != NULL ) { saved_item.path = *path; *path = NULL; }
	saved_item.row_align = row_align;
}

void free_saved_item( void )
{
	if ( saved_item.menu != NULL )
	{
		gtk_menu_detach( saved_item.menu );
		/* g_object_unref( saved_item.menu ); */
		saved_item.menu = NULL;
	}

	g_free( saved_item.path );
	saved_item.path = NULL;

	g_free( saved_item.didl );
	saved_item.didl = NULL;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean is_local_folder( const char *object_id )
{
	return ( strncmp( object_id, local_id, sizeof( local_id ) - 1 ) == 0 );
}

const char *get_local_title( void )
{
	return ( Settings->browser.local_folder ) ? Text( BROWSER_LOCAL_FOLDER ) : NULL;
}

struct xml_content *get_local_folder( const char *id, GError **error )
{
#ifndef MAEMO
	GString *playlist = g_string_new( "" );
	const gchar *album_art_filename;
	GList *dir, *l;
	gchar *path;
	int n = 0;

	ASSERT( id != NULL );
	TRACE(( "get_local_folder( \"%s\" )...\n", id ));

	path = get_local_folder_path( id );
	dir = get_sorted_dir_content( path, &album_art_filename, error );
	g_free( path );

	for ( l = dir; l != NULL; l = l->next )
	{
		struct dir_entry *entry = (struct dir_entry *)l->data;

		if ( (entry->st_mode & S_IFDIR) != 0 )
		{
			path = xml_wrap_delimiters( entry->filename );
			g_string_append_printf( playlist, "<item id=\"%s:%s\">", local_id, path );
			g_free( path );

			xml_string_append_boxed( playlist, "upnp:class", "object.container.localFolder", NULL );
			xml_string_append_boxed( playlist, "dc:title", entry->name, NULL );

			g_string_append( playlist, "</item>" );
			n++;
		}
		else if ( (entry->st_mode & S_IFREG) != 0 )
		{
			if ( GetMetadataFromFile( entry->filename, album_art_filename, playlist, FALSE, NULL ) ) n++;
		}
	}

	free_sorted_dir_content( dir );

	return xml_content_from_string( playlist, n );
#else
	return NULL;
#endif
}

gchar *get_local_folder_path( const char *id )
{
	if ( id[sizeof(local_id)-1] == '\0' )
		return g_strdup( Settings->browser.local_folder_path );
	else if ( id[sizeof(local_id)-1] == ':' )
		return xml_unwrap_delimiters( id + sizeof(local_id) );
	else
		return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

static const char user_radio_filename[] = "radio.xml";

gboolean is_radio_folder( const char *object_id )
{
	return ( strcmp( object_id, radio_id ) == 0 );
}

const char *get_radio_title( void )
{
	return Text( RADIO );
}

struct xml_content *get_radio_folder( GError **error )
{
	gchar *filename;

	int tracks_not_added;

	filename = BuildUserConfigFilename( user_radio_filename );
	if ( !g_file_test( filename, G_FILE_TEST_IS_REGULAR ) )
	{
		gchar *basename = g_strdup_printf( "radio_%s.xml", GetLanguage() );
		filename = BuildApplDataFilename( basename );
		g_free( basename );
		if ( !g_file_test( filename, G_FILE_TEST_IS_REGULAR ) )
		{
			filename = BuildApplDataFilename( "radio.xml" );
		}
	}

	return LoadPlaylist( filename, &tracks_not_added, error );
}

/*-----------------------------------------------------------------------------------------------*/

gboolean is_radiotime_folder( const char *object_id )
{
	return ( strncmp( object_id, radiotime_id, sizeof(radiotime_id) - 1 ) == 0 );
}

/*-----------------------------------------------------------------------------------------------*/

gboolean is_lastfm_folder( const char *object_id )
{
	return ( strcmp( object_id, lastfm_id ) == 0 );
}

struct lastfm_dialog_data
{
	GtkLabel *label;
	GString *str[NUM_LASTFM_KEYS];
};

struct lastfm_play_data
{
	struct animation *ani;
	gchar *didl_item;
	char *lastfm_playlist;
	gchar *upnp_playlist;
	GError *error;
};

void lastfm_key_changed( GtkComboBox *widget, struct lastfm_dialog_data *data )
{
	gint i = gtk_combo_box_get_active( widget );

	TRACE(( "# lastfm_key_changed\n" ));

	gtk_label_set_text( data->label, data->str[i]->str );
}

void new_lastfm( void )
{
	static enum LastfmKey lastfm_dialog_key;
	struct lastfm_dialog_data data = {0};
	GtkDialog *dialog;
	GtkWidget *widget, *key, *value;
	GtkTable *table;
	const gchar *str;
	gint i;

	dialog = new_modal_dialog_with_ok_cancel( Text(LASTFM_RADIO), NULL );

	table = new_table( 2, 2 );

	widget = new_label_with_colon( Text(LASTFM_RADIO_TYPE) );
	attach_to_table( table, widget, 0, 0, 1 );

	key = gtk_combo_box_new_text();
	for ( i = 0; i < NUM_LASTFM_KEYS; i++ )
	{
		const char *radio;

		data.str[i] = g_string_new( "" );
		radio = GetLastfmRadioItem( i, NULL, data.str[i] );
		if ( radio == NULL ) break;
		g_string_append( data.str[i], ":" );

		gtk_combo_box_append_text( GTK_COMBO_BOX(key), radio );
	}
	attach_to_table( table, key, 0, 1, 2 );

	str = Text(LASTFM_ARTIST);
	if ( strlen( Text(LASTFM_USER) ) > strlen( str ) )
		str = Text(LASTFM_USER);
	if ( strlen( Text(LASTFM_TAG) ) > strlen( str ) )
		str = Text(LASTFM_TAG);

	widget = new_label_with_colon( str );
	attach_to_table( table, widget, 1, 0, 1 );
	data.label = GTK_LABEL(widget);

	value = new_entry( BROWSER_MAX_LASTFM_NAME );
	attach_to_table( table, value, 1, 1, 2 );

	gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(table) );
	gtk_widget_show_all( GTK_WIDGET(dialog) );

	g_signal_connect( G_OBJECT(key), "changed", G_CALLBACK(lastfm_key_changed), &data );
	gtk_combo_box_set_active( GTK_COMBO_BOX(key), lastfm_dialog_key );

	if ( gtk_dialog_run( dialog ) == GTK_RESPONSE_OK )
	{
		const gchar *text = gtk_entry_get_text( GTK_ENTRY( value ) );
		if ( text != NULL && *text != '\0' )
		{
			i = gtk_combo_box_get_active( GTK_COMBO_BOX(key) );

			busy_enter();

			g_string_assign( data.str[i], "" );
			VERIFY( GetLastfmRadioItem( i, text, data.str[i] ) != NULL );
			play_lastfm( data.str[i]->str );

			busy_leave();

			lastfm_dialog_key = i;
		}
	}

	for ( i = 0; i < NUM_LASTFM_KEYS; i++ )
	{
		g_string_free( data.str[i], TRUE );
	}

	gtk_widget_destroy( GTK_WIDGET(dialog) );
}

void add_lastfm( const char *didl_item )
{
	char dtostr_buf[G_ASCII_DTOSTR_BUF_SIZE];

	busy_enter();

	VERIFY( AddToLastfmFolder( didl_item ) );
	replace_current_path( align_path, g_ascii_dtostr( dtostr_buf, G_ASCII_DTOSTR_BUF_SIZE, 0.0 ) );
	browser_refresh();

	busy_leave();
}

gboolean play_lastfm( const char *didl_item )
{
	gboolean result = FALSE;

	if ( ready_to_replace() )
	{
		struct lastfm_play_data *data = g_new0( struct lastfm_play_data, 1 );
		gchar *title = GetTitle( didl_item, NULL, 0 );
		GError *error = NULL;

		data->ani = show_animation( Text(LASTFM_RADIO), title, BROWSER_BUSY_TIMEOUT );
		data->didl_item = g_strdup( didl_item );
		if ( g_thread_create( play_lastfm_thread, data, FALSE, &error ) != NULL )
			result = TRUE;

		g_free( title );

		HandleError( error, Text(ERROR_PLAY_LASTFM_RADIO) );
	}

	return result;
}

gpointer play_lastfm_thread( gpointer user_data )
{
	struct lastfm_play_data *data = (struct lastfm_play_data *)user_data;

	char *tune_result;
	gchar *id;

	id = xml_get_named_attribute_str( data->didl_item, "id" );
	tune_result = lastfm_radio_tune( GetLanguage(), id, GetLastfmSessionKey(), &data->error );
	g_free( id );
	if ( tune_result != NULL )
	{
		TRACE(( "\n" ));
		TRACE(( "\n" ));
		TRACE(( "lastfm_radio_tune() = %s\n", tune_result ));
		TRACE(( "\n" ));
		TRACE(( "\n" ));

		upnp_free_string( tune_result );

		if ( !IsStopped() ) Stop( &data->error );

		if ( data->error == NULL && ShufflePlay )
		{
			TRACE(( "play_lastfm_thread(): Clear playlist shuffling...\n" ));
			SetShuffle( FALSE, &data->error );
		}

		if ( data->error == NULL )
		{
			if ( PlaylistClear( ENTER_GDK_THREADS, &data->error ) )
			{
				if ( http_start_server( &data->error ) )
				{
					return play_lastfm_thread_get( data );
				}
			}
		}
	}

	cancel_animation( data->ani );
	g_idle_add( play_lastfm_thread_end, data );
	return NULL;
}

gpointer play_lastfm_thread_get( gpointer user_data )
{
	struct lastfm_play_data *data = (struct lastfm_play_data *)user_data;

	data->lastfm_playlist = lastfm_radio_get_playlist( GetLastfmSessionKey(), Settings->lastfm.discovery_mode, &data->error );
	if ( data->lastfm_playlist != NULL )
	{
		gchar *title_strip;

	/*
		TRACE(( "\n" ));
		TRACE(( "\n" ));
		TRACE(( "lastfm_radio_get_playlist() = %s\n", data->lastfm_playlist ));
		TRACE(( "\n" ));
		TRACE(( "\n" ));

		g_file_set_contents( "lastfm_radio_playlist.xml", data->lastfm_playlist, -1, NULL );
	*/
		title_strip = /*( data->didl_item != NULL && upnp_device_is( GetAVTransport(), linn_serviceId_Info ) )
			?*/ g_strdup_printf( Text(LASTFM_TITLE_STRIP), GetTitle( data->didl_item, NULL, 0 ) )
			/*: NULL*/;
		data->upnp_playlist = LastfmRadioPlaylistToUpnpPlaylist( title_strip, data->lastfm_playlist );
		ASSERT( data->upnp_playlist != NULL );
		g_free( title_strip );
	}

	cancel_animation( data->ani );
	g_idle_add( play_lastfm_thread_end, data );
	return NULL;
}

gboolean play_lastfm_thread_end( gpointer user_data )
{
	struct lastfm_play_data *data = (struct lastfm_play_data *)user_data;

	gdk_threads_enter();

	if ( data->upnp_playlist != NULL )
	{
		if ( data->didl_item != NULL )
		{
			gchar *title = GetInfo( data->lastfm_playlist, "title", NULL, 0 );
			if ( title == NULL ) title = GetTitle( data->didl_item, NULL, 0 );

			upnp_free_string( data->lastfm_playlist );
			AddToPlaylist( data->upnp_playlist, title );
			g_free( title );
			g_free( data->upnp_playlist );

			ClearNowPlaying( TRUE );
			SetCurrentPage( Settings->browser.page_after_replacing_playlist );

			add_lastfm( data->didl_item );
		}
		else
		{
			int track_count = GetTrackCount();
			const int n = 5;

			upnp_free_string( data->lastfm_playlist );
			AddToPlaylist( data->upnp_playlist, NULL );
			g_free( data->upnp_playlist );

			if ( track_count > n )  /* Limit track history to <n> tracks */
			{
				TRACE(( "play_lastfm_thread_end(): track_count = %d > %d --> PlaylistDelete( %d )\n", track_count, n, track_count - n ));
				PlaylistDelete( 0, track_count - n, NULL, &data->error );
			}
		}
	}
	else
	{
		ASSERT( data->lastfm_playlist == NULL );
		upnp_free_string( data->lastfm_playlist );
	}

	g_free( data->didl_item );
	clear_animation( data->ani );

	HandleError( data->error, Text(ERROR_PLAY_LASTFM_RADIO) );
	g_free( data );

	gdk_threads_leave();

	return FALSE;
}

void GetLastfmRadioTracks( void )
{
	struct lastfm_play_data *data = g_new0( struct lastfm_play_data, 1 );
	GError *error = NULL;

	TRACE(( "GetLastfmRadioTracks()..." ));
	g_thread_create( play_lastfm_thread_get, data, FALSE, &error );
	HandleError( error, Text(ERROR_PLAY_LASTFM_RADIO) );
}

/*-----------------------------------------------------------------------------------------------*/

void update_browser_menu( void )
{
	gboolean sensitive = can_go_back_in_path();
	gtk_widget_set_sensitive( GTK_WIDGET(mi_go_back), sensitive );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_go_back), sensitive );

	gtk_widget_set_sensitive( GTK_WIDGET(mi_refresh), id_path != NULL && id_path->str[0] != '\0' );
}

gboolean go_back( void )
{
	if ( can_go_back_in_path() )
	{
		go_back_in_path( id_path );
		go_back_in_path( title_path );
		go_back_in_path( tree_path );
		go_back_in_path( align_path );
		return TRUE;
	}

	return FALSE;
}

void browser_go_back( void )  /* (GtkToolButton *toolbutton, gpointer user_data)*/
{
	TRACE(( "## browser_go_back()\n" ));

	if ( go_back() ) start_browsing( TRUE );
}

void browser_refresh( void )
{
	const char *object_id = get_current_path( id_path );

	TRACE(( "## browser_refresh(): ObjectID = \"%s\"\n", (object_id != NULL) ? object_id : "<NULL>" ));

	if ( object_id != NULL && object_id[0] != '\0' )
	{
		const char *device_id = get_device_id( object_id );

		remove_from_browser_cache( device_id, object_id );
		start_browsing( TRUE );
	}
}

static gboolean browser_radio_dialog( const char *caption, GtkWindow *parent, gchar **title, gchar **genre, gchar **url, gchar **album_art_uri )
{
	gboolean result = FALSE;

	GtkWidget *title_widget, *genre_widget, *url_widget, *album_art_uri_widget;
	GtkDialog *dialog;
	GtkTable *table;

	dialog = new_modal_dialog_with_ok_cancel( caption, parent );
	set_dialog_size_request( dialog, FALSE );
	table = new_table( 4, 2 );

	attach_to_table( table, new_label_with_colon( Text(DC_TITLE) ), 0, 0, 1 );
	title_widget = gtk_entry_new();
	if ( *title != NULL ) gtk_entry_set_text( GTK_ENTRY(title_widget), *title );
	attach_to_table( table, title_widget, 0, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(UPNP_GENRE) ), 1, 0, 1 );
	genre_widget = gtk_entry_new();
	if ( *genre != NULL ) gtk_entry_set_text( GTK_ENTRY(genre_widget), *genre );
	attach_to_table( table, genre_widget, 1, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(INFO_URL) ), 2, 0, 1 );
	url_widget = gtk_entry_new();
	if ( *url != NULL ) gtk_entry_set_text( GTK_ENTRY(url_widget), *url );
	attach_to_table( table, url_widget, 2, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(RADIO_ALBUM_ART_URI) ), 3, 0, 1 );
	album_art_uri_widget = gtk_entry_new();
	if ( *album_art_uri != NULL ) gtk_entry_set_text( GTK_ENTRY(album_art_uri_widget), *album_art_uri );
	attach_to_table( table, album_art_uri_widget, 3, 1, 2 );

	gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(table) );
	gtk_widget_show_all( GTK_WIDGET(dialog) );
	if ( gtk_dialog_run( dialog ) == GTK_RESPONSE_OK )
	{
		const gchar *new_title = gtk_entry_get_text( GTK_ENTRY(title_widget) );
		const gchar *new_genre = gtk_entry_get_text( GTK_ENTRY(genre_widget) );
		const gchar *new_url   = gtk_entry_get_text( GTK_ENTRY(url_widget) );
		const gchar *new_album_art_uri = gtk_entry_get_text( GTK_ENTRY(album_art_uri_widget) );

		if ( *title == NULL || strcmp( *title, new_title ) != 0 )
		{
			g_free( *title );
			*title = g_strdup( new_title );
			result = TRUE;
		}
		if ( *genre == NULL || strcmp( *genre, new_genre ) != 0 )
		{
			g_free( *genre );
			*genre = g_strdup( new_genre );
			result = TRUE;
		}
		if ( *url == NULL || strcmp( *url, new_url ) != 0 )
		{
			g_free( *url );
			*url = g_strdup( new_url );
			result = TRUE;
		}
		if ( *album_art_uri == NULL || strcmp( *album_art_uri, new_album_art_uri ) != 0 )
		{
			g_free( *album_art_uri );
			*album_art_uri = g_strdup( new_album_art_uri );
			result = TRUE;
		}
	}

	gtk_widget_destroy( GTK_WIDGET(dialog) );
	return result;
}

static void add_to_browser_radio( GtkListStore *store, char *content )
{
	xml_iter iter;
	char *item;

	for ( item = xml_first( content, &iter ); item != NULL; item = xml_next( &iter ) )
	{
		GtkTreeIter iter;

		gchar *title = xml_get_named_str( item, "dc:title" );
		gchar *genre = xml_get_named_str( item, "upnp:genre" );
		gchar *url = xml_get_named_str( item, "res" );
		gchar *album_art_uri = xml_get_named_str( item, "upnp:albumArtURI" );

		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter,
			0, title, 1, genre, 2, url, 3, album_art_uri, -1 );

		g_free( album_art_uri );
		g_free( url );
		g_free( genre );
		g_free( title );
	}
}

static void set_browser_radio_data_changed( GtkTreeView *view )
{
	g_object_set_data( G_OBJECT(view), "data-changed", (gpointer)radioId );
}

static gboolean save_browser_radio_iter( FILE *fp, GtkTreeModel *model, GtkTreeIter *iter, GError **error )
{
	extern void append_audio_broadcast_item( GString *content_str, const char *title, const char *res, const char *album_art_uri, const char *genre );

	gchar *title = NULL, *genre = NULL, *url = NULL, *album_art_uri = NULL;
	GString *item;
	gboolean flag;

	gtk_tree_model_get( model, iter, 0, &title, 1, &genre, 2, &url, 3, &album_art_uri, -1 );

	item = g_string_new( "" );
	append_audio_broadcast_item( item, title, url, album_art_uri, genre );
	flag = WritePlaylistItem( fp, item->str, error );
	g_string_free( item, TRUE );

	g_free( album_art_uri );
	g_free( url );
	g_free( genre );
	g_free( title );

	return flag;
}

static void save_browser_radio( const char *filename, GtkTreeView *view, gboolean save_selection_only )
{
	GError *error = NULL;
	FILE *fp;

	TRACE(( "save_browser_radio(): Saving %s...\n", filename ));

	fp = CreatePlaylistFile( filename, FALSE, &error );
	if ( fp != NULL )
	{
		GtkTreeModel *model = gtk_tree_view_get_model( view );
		GtkTreeIter iter;
		gboolean flag;

		if ( save_selection_only )
		{
			GtkTreeSelection *selection = gtk_tree_view_get_selection( view );
			GList *rows = gtk_tree_selection_get_selected_rows( selection, NULL );
			GList *row;

			for ( row = rows; row != NULL; row = row->next )
			{
				GtkTreePath *path = (GtkTreePath *)row->data;

				if ( gtk_tree_model_get_iter( model, &iter, path ) )
				{
					if ( !save_browser_radio_iter( fp, model, &iter, &error ) )
						row = NULL;
				}
			}

			g_list_foreach( rows, (GFunc)gtk_tree_path_free, NULL );
			g_list_free( rows );
		}
		else
		{
			for ( flag = gtk_tree_model_get_iter_first( model, &iter ); flag; flag = gtk_tree_model_iter_next( model, &iter ) )
			{
				flag = save_browser_radio_iter( fp, model, &iter, &error );
			}
		}

		flag = ClosePlaylistFile( fp, &error );
		if ( !flag ) remove( filename );
	}

	HandleError( error, Text(ERROR_PLAYLIST_SAVE), filename );
}

struct browser_radio_data
{
	GtkDialog *dialog;
	GtkTreeView *view;
	GtkListStore *store;
};

static void browser_radio_new( GtkButton *button, struct browser_radio_data *data )
{
	gchar *title, *genre, *url, *album_art_uri;

	TRACE(( "# browser_radio_new()\n" ));

	title = genre = url = album_art_uri = NULL;

	if ( browser_radio_dialog( Text(RADIO_NEW), GTK_WINDOW(data->dialog), &title, &genre, &url, &album_art_uri ) )
	{
		GtkTreeIter iter;

		gtk_list_store_append( data->store, &iter );
		gtk_list_store_set( data->store, &iter, 0, title, 1, genre, 2, url, 3, album_art_uri, -1 );

		set_browser_radio_data_changed( data->view );
	}

	g_free( album_art_uri );
	g_free( url );
	g_free( genre );
	g_free( title );
}

static void browser_radio_activated( GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, struct browser_radio_data *data )
{
	GtkTreeIter iter;

	if ( gtk_tree_model_get_iter( GTK_TREE_MODEL(data->store), &iter, path ) )
	{
		gchar *title = NULL, *genre = NULL, *url = NULL, *album_art_uri = NULL;

		gtk_tree_model_get( GTK_TREE_MODEL(data->store), &iter, 0, &title, 1, &genre, 2, &url, 3, &album_art_uri, -1 );

		if ( browser_radio_dialog( Text(RADIO_EDIT), GTK_WINDOW(data->dialog), &title, &genre, &url, &album_art_uri ) )
		{
			gtk_list_store_set( data->store, &iter, 0, title, 1, genre, 2, url, 3, album_art_uri, -1 );

			set_browser_radio_data_changed( data->view );
		}

		g_free( album_art_uri );
		g_free( url );
		g_free( genre );
		g_free( title );
	}
}

static void browser_radio_edit( GtkButton *button, struct browser_radio_data *data )
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection( data->view );
	GList *rows;

	TRACE(( "# browser_radio_edit()\n" ));

	rows = gtk_tree_selection_get_selected_rows( selection, NULL );
	if ( rows != NULL )
	{
		gtk_tree_view_row_activated( data->view, rows->data, NULL );

		g_list_foreach( rows, (GFunc)gtk_tree_path_free, NULL );
		g_list_free( rows );
	}
}

static void browser_radio_remove( GtkButton *button, struct browser_radio_data *data )
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection( data->view );
	GList *rrs, *l;

	TRACE(( "# browser_radio_remove()\n" ));

	rrs = get_selected_tree_row_references( selection );
	for ( l = rrs; l != NULL; l = l->next )
	{
		GtkTreeRowReference *rr = (GtkTreeRowReference *)l->data;
		GtkTreePath *path = gtk_tree_row_reference_get_path( rr );
		GtkTreeIter iter;

		if ( gtk_tree_model_get_iter( GTK_TREE_MODEL(data->store), &iter, path ) )
		{
			gtk_list_store_remove( data->store, &iter );

			set_browser_radio_data_changed( data->view );
		}

		gtk_tree_row_reference_free( rr );
	}
	g_list_free( rrs );
}

static gchar *current_playlist_folder;

static void browser_radio_import( GtkButton *button, struct browser_radio_data *data )
{
	gchar *filename;

	TRACE(( "# browser_radio_import()\n" ));

	filename = ChoosePlaylistFile( GTK_WINDOW(data->dialog), NULL, &current_playlist_folder );
	if ( filename != NULL )
	{
		struct xml_content *content;
		int tracks_not_added;
		GError *error = NULL;

		content = LoadPlaylist( filename, &tracks_not_added, &error );
		if ( content != NULL )
		{
			add_to_browser_radio( data->store, content->str );
			free_xml_content( content );

			set_browser_radio_data_changed( data->view );
		}

		g_free( filename );
		HandleError( error, Text(ERROR_PLAYLIST_LOAD) );
	}
}

static void browser_radio_export( GtkButton *button, struct browser_radio_data *data )
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection( data->view );
	gchar *current_name = g_strdup( Text(RADIO) );
	gboolean save_selection_only = FALSE;
	gint response = GTK_RESPONSE_OK;
	GList *rows;

	TRACE(( "# browser_radio_export()\n" ));

	rows = gtk_tree_selection_get_selected_rows( selection, NULL );
	if ( rows != NULL )
	{
		GtkDialog *dialog;
		GtkTable *table;
		GtkWidget *widget;

		dialog = new_modal_dialog_with_ok_cancel( "", GTK_WINDOW(data->dialog) );
		gtk_dialog_set_has_separator( dialog, FALSE );
		table = new_table( 3, 2 );
		gtk_table_set_row_spacings( table, 0 );
		gtk_table_set_col_spacings( table, 3 * SPACING );

		widget = gtk_image_new_from_stock( GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG );
		attach_to_table( table, widget, 0, 0, 1 );

		widget = gtk_label_new( Text(RADIO_EXPORT_WHICH) );
		gtk_misc_set_alignment( GTK_MISC(widget), 0, (gfloat)0.4 );
		attach_to_table( table, widget, 0, 1, 2 );

		widget = gtk_radio_button_new_with_label( NULL, Text(RADIO_EXPORT_ALL) );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), FALSE );
		attach_to_table( table, widget, 1, 1, 2 );

		widget = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(widget), Text(RADIO_EXPORT_SELECTION) );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widget), TRUE );
		attach_to_table( table, widget, 2, 1, 2 );

		gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(table) );
		gtk_widget_show_all( GTK_WIDGET(dialog) );
		response = gtk_dialog_run( dialog );
		save_selection_only = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widget) );
		gtk_widget_destroy( GTK_WIDGET(dialog) );

		TRACE(( "browser_radio_export(): save_selection_only = %s\n", save_selection_only ? "TRUE" : "FALSE" ));

		if ( save_selection_only )
		{
			guint n = g_list_length( rows ), i;
			gchar **titles = g_new0( gchar *, n );
			gchar *common_part;
			GList *row;

			for ( i = 0, row = rows; i < n && row != NULL; i++, row = row->next )
			{
				GtkTreePath *path = (GtkTreePath *)row->data;
				GtkTreeIter iter;

				if ( gtk_tree_model_get_iter( GTK_TREE_MODEL(data->store), &iter, path ) )
					gtk_tree_model_get( GTK_TREE_MODEL(data->store), &iter, 0, &titles[i], -1 );
			}

			common_part = make_valid_filename( get_common_str_part( titles, n ) );
			if ( *common_part != '\0' )
			{
				g_free( current_name );
				current_name = common_part;
			}
			else
			{
				g_free( common_part );
			}

			for ( i = 0; i < n; i++ ) g_free( titles[i] );
			g_free( titles );
		}

		g_list_foreach( rows, (GFunc)gtk_tree_path_free, NULL );
		g_list_free( rows );
	}

	if ( response == GTK_RESPONSE_OK )
	{
		gchar *filename = ChoosePlaylistFile( GTK_WINDOW(data->dialog), current_name, &current_playlist_folder );

		if ( filename != NULL )
		{
			save_browser_radio( filename, data->view, save_selection_only );
			g_free( filename );
		}
	}

	g_free( current_name );
}

void browser_radio( void )
{
	struct xml_content *result;
	GError *error = NULL;

	result = get_radio_folder( &error );
	if ( result != NULL )
	{
		struct browser_radio_data data;
		GtkBox *hbox, *vbox;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;
		GtkWidget *widget;

		data.dialog = new_modal_dialog_with_ok_cancel( Text(RADIO), NULL );
		set_dialog_size_request( data.dialog, TRUE );

		hbox = new_hbox();

		data.store = gtk_list_store_new( 4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );
		add_to_browser_radio( data.store, result->str );

		free_xml_content( result );

		data.view = GTK_TREE_VIEW( gtk_tree_view_new_with_model( GTK_TREE_MODEL( data.store ) ) );
		gtk_tree_view_set_headers_visible( data.view, FALSE /* TRUE */);
		gtk_tree_view_set_enable_search( data.view, TRUE );
		gtk_tree_view_set_search_column( data.view, 0 );
		gtk_tree_sortable_set_sort_column_id( GTK_TREE_SORTABLE( data.store ), 0, GTK_SORT_ASCENDING );
		g_object_unref( data.store );

		gtk_tree_selection_set_mode( gtk_tree_view_get_selection( data.view ), GTK_SELECTION_MULTIPLE );
		g_signal_connect( G_OBJECT(data.view), "row-activated", GTK_SIGNAL_FUNC(browser_radio_activated), &data );

		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start( column, renderer, FALSE );
		gtk_tree_view_column_set_attributes( column, renderer, "text", 0, NULL );
		gtk_tree_view_column_set_title( column, Text(DC_TITLE) );
		gtk_tree_view_append_column( data.view, column );
/*
		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start( column, renderer, FALSE );
		gtk_tree_view_column_set_attributes( column, renderer, "text", 1, NULL );
		gtk_tree_view_column_set_title( column, Text(UPNP_GENRE) );
		gtk_tree_view_append_column( data.view, column );
*/
		vbox = GTK_BOX( gtk_vbox_new( FALSE, SPACING ) );
		widget = gtk_button_new_from_stock( GTK_STOCK_NEW );
		g_signal_connect( widget, "clicked", G_CALLBACK(browser_radio_new), &data );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );
		widget = gtk_button_new_from_stock( GTK_STOCK_EDIT );
		g_signal_connect( widget, "clicked", G_CALLBACK(browser_radio_edit), &data );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );
		widget = gtk_button_new_from_stock( GTK_STOCK_REMOVE );
		g_signal_connect( widget, "clicked", G_CALLBACK(browser_radio_remove), &data );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );

		widget = gtk_hbox_new( FALSE, 0 );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );
		widget = gtk_hbox_new( FALSE, 0 );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );
		widget = gtk_hbox_new( FALSE, 0 );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );

		widget = gtk_button_new_with_label( Text(RADIO_IMPORT) );
		g_signal_connect( widget, "clicked", G_CALLBACK(browser_radio_import), &data );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );
		widget = gtk_button_new_with_label( Text(RADIO_EXPORT) );
		g_signal_connect( widget, "clicked", G_CALLBACK(browser_radio_export), &data );
		gtk_box_pack_start( vbox, widget, FALSE, FALSE, 0 );

		widget = new_scrolled_window();
		gtk_container_add( GTK_CONTAINER( widget ), GTK_WIDGET( data.view ) );

		gtk_box_pack_start( hbox, widget, TRUE, TRUE, 0 );
		gtk_box_pack_start( hbox, GTK_WIDGET( vbox ), FALSE, FALSE, 0 );
		gtk_box_pack_start_defaults( GTK_BOX(data.dialog->vbox), GTK_WIDGET(hbox) );

		gtk_widget_show_all( GTK_WIDGET(data.dialog) );
		if ( gtk_dialog_run( data.dialog ) == GTK_RESPONSE_OK && g_object_get_data( G_OBJECT(data.view), "data-changed" ) != NULL )
		{
			gchar *filename = BuildUserConfigFilename( user_radio_filename );
			save_browser_radio( filename, data.view, FALSE );
			g_free( filename );

			if ( is_radio_folder( get_current_path( id_path ) ) )
				browser_refresh();
		}

		gtk_widget_destroy( GTK_WIDGET(data.dialog) );
	}

	HandleError( error, Text(ERROR_BROWSE_FOLDER), Text(RADIO) );
}

/*-----------------------------------------------------------------------------------------------*/

gboolean browser_button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	TRACE(( "## browser_button_press_event( button = %d )\n", event->button ));

	if ( browser_row_activated_by_button == 0 )
	{
		browser_row_activated_button = event->button;
		gtk_widget_get_pointer( widget, NULL, &browser_row_activated_y );
		browser_row_activated_by_button++;
	}
	else
	{
		browser_row_activated_by_button = 0;
	}

	return FALSE;
}

static gboolean activate_browser_row( gpointer user_data )
{
	gdk_threads_enter();

	if ( browser_row_activated_by_button == 3 && browser_row_activated_path != NULL )
	{
		gtk_tree_view_row_activated( browser, browser_row_activated_path, NULL );

		gtk_tree_path_free( browser_row_activated_path );
		browser_row_activated_path = NULL;
	}

	browser_row_activated_by_button = 0;

	gdk_threads_leave();
	return FALSE;
}

gboolean browser_button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	TRACE(( "## browser_button_release_event()\n" ));

	if ( browser_row_activated_by_button == 2 && event->button == browser_row_activated_button && browser_row_activated_path != NULL )
	{
		GdkRectangle rect;
		gint y;

		gtk_widget_get_pointer( widget, NULL, &y );
		gtk_tree_view_get_cell_area( browser, browser_row_activated_path, NULL, &rect );

		TRACE(( "browser_row_activated_y = %d, y = %d, rect.height = %d\n", browser_row_activated_y, y, rect.height ));

		if ( abs( y - browser_row_activated_y ) < rect.height )
		{
			browser_row_activated_by_button++;

			if ( browser_row_activated_button == 1 /* left click */ )
			{
				if ( Settings->browser.activate_items_with_single_click )
				{
					/*gtk_tree_view_row_activated( browser, browser_row_activated_path, NULL );*/
					g_timeout_add( 100, activate_browser_row, NULL /*user_data*/ );
					return FALSE;
				}
			}
			else if ( browser_row_activated_button == 3 /* right click */ )
			{
				gtk_tree_view_row_activated( browser, browser_row_activated_path, NULL );
			}
		}

		gtk_tree_path_free( browser_row_activated_path );
		browser_row_activated_path = NULL;
	}

	browser_row_activated_by_button = 0;

	return FALSE;
}

gboolean browser_key_press_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	GtkTreePath *path = NULL;

	TRACE(( "## browser_key_press_event()\n" ));

	if ( event->keyval == GDK_Up || event->keyval == GDK_Down )
	{
		gtk_tree_view_get_cursor( browser, &path, NULL );
		TRACE(( "browser_key_press_event(): path = %p\n", (gpointer)path ));

		if ( path == NULL )
		{
			path = gtk_tree_path_new_from_string( "0" );  /* Select first item */
			gtk_tree_view_set_cursor( browser, path, NULL, FALSE );
			gtk_tree_path_free( path );
			return TRUE;
		}

		gtk_tree_path_free( path );
	}

	return KeyPressEvent( widget, event, GINT_TO_POINTER(BROWSER_PAGE) );
}

gboolean browser_key_release_event( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	TRACE(( "## browser_key_release_event()\n" ));

	return KeyReleaseEvent( widget, event, GINT_TO_POINTER(BROWSER_PAGE) );
}

void browser_selection_changed( GtkTreeSelection *selection, gpointer user_data )
{
	TRACE(( "## browser_selection_changed()\n" ));

	if ( browser_row_activated_path != NULL )
	{
		gtk_tree_path_free( browser_row_activated_path );
		browser_row_activated_path = NULL;
	}

	if ( browser_row_activated_by_button == 1 )

	{
		GtkTreeModel *model;
		GtkTreeIter iter;

		if ( gtk_tree_selection_get_selected( selection, &model, &iter ) )
		{
			browser_row_activated_path = gtk_tree_model_get_path( model, &iter );
			browser_row_activated_by_button++;
			return;
		}
	}

	browser_row_activated_by_button = 0;
}

static gfloat get_row_align( GtkTreeView *view, GtkTreePath *path, GtkScrolledWindow *window )
{
	GdkRectangle cell_rect, vis_rect;

	gtk_tree_view_get_background_area( view, path, NULL, &cell_rect );
	cell_rect.y += (int)gtk_adjustment_get_value( gtk_scrolled_window_get_vadjustment( window ) );
	gtk_tree_view_get_visible_rect( view, &vis_rect );
	TRACE(( "cell.y/height = %d/%d, vis.y/height = %d/%d\n", cell_rect.y, cell_rect.height, vis_rect.y, vis_rect.height ));

	return (gfloat)(cell_rect.y - vis_rect.y) / (vis_rect.height - cell_rect.height);
}

void browser_row_activated( GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data )
{
	TRACE(( "## browser_row_activated()\n" ));
	if ( !row_activated_flag )
	{
		const gchar *error_format = Text(ERROR_HANDLING_ITEM);
		GError *error = NULL;

		gchar *path_str = gtk_tree_path_to_string( path );
		gchar *didl_item = get_browser_item( view, path, path_str );
		gchar *title;

		if ( didl_item != NULL )
		{
			set_row_activated_flag();

			title = GetTitle( didl_item, NULL, 0 );

			ASSERT( didl_item != NULL );
			if ( didl_item != NULL )
			{
				const char *class_id = get_upnp_class_id( didl_item );
				int action = BROWSER_NO_ACTION;

				TRACE(( "gtk_tree_model_get() ok, class_id = \"%s\"\n", (class_id != NULL) ? class_id : "<NULL>" ));

				if ( browser_row_activated_by_button == 3 && browser_row_activated_button == 3 )
				{
					/* Right mouse click => Show popup menu */
					action = BROWSER_POPUP_MENU;
				}

				else if ( class_id == mediaServerId || class_id == containerId || class_id == localFolderId ||
						  class_id == localId || class_id == radioId || class_id == radiotimeId || class_id == lastfmId )
				{
					action = BROWSER_OPEN;
				}

				else if ( class_id == playlistContainerId )
				{
					action = Settings->browser.playlist_container_action;
				}
				else if ( class_id == musicAlbumId )
				{
					action = Settings->browser.music_album_action;
				}
				else if ( class_id == musicTrackId )
				{
					action = Settings->browser.music_track_action;
				}
				else if ( class_id == audioBroadcastId )
				{
					if ( IsLastfmItem( didl_item ) )
						action = BROWSER_PLAY_LASTFM;
					else
						action = Settings->browser.audio_broadcast_action;
				}
				else if ( class_id == audioItemId )
				{
					char id[8];  /* A Radio Time Preset item from Linn has an empty "id" */

					TRACE(( "browser_row_activated(): is_radiotime_folder() = %d\n", is_radiotime_folder( get_current_path( id_path ) ) ));
					TRACE(( "browser_row_activated(): id = \"%s\"\n", xml_get_named_attribute( didl_item, "id", id, sizeof( id ) ) ));

					if ( is_radiotime_folder( get_current_path( id_path ) ) &&
						 (xml_get_named_attribute( didl_item, "id", id, sizeof( id ) ) == NULL || id[0] == '\0') )
						action = Settings->browser.audio_broadcast_action;
					else
						action = BROWSER_POPUP_MENU;
				}

				else if ( class_id == goBackId )
				{
					browser_go_back();
				}

				else if ( class_id == newId )
				{
					if ( IsLastfmItem( didl_item ) )
					{
						new_lastfm();
					}
					else
					{
						g_set_error( &error, 0, 2, Text(ERRMSG_ITEM_ACTIVATED) );
					}
				}
				else
				{
					g_set_error( &error, 0, 1, Text(ERRMSG_ITEM_ACTIVATED) );
				}

				if ( action >= 0 )
				{
					gfloat row_align = get_row_align( view, path, scrolled_browser );
					browser_action( action, &didl_item, &path_str, row_align );
				}
			}
			else
			{
				g_set_error( &error, 0, 3, Text(ERRMSG_INTERNAL) );
			}

			HandleError( error, error_format, title );
			g_free( title );

			g_free( didl_item );
			g_free( path_str );
		}
	}

	browser_row_activated_by_key = FALSE;
	browser_row_activated_by_button = 0;
}

gchar *get_browser_item( GtkTreeView *view, GtkTreePath *path, const gchar *path_str )
{
	GtkTreeModel *model = gtk_tree_view_get_model( view );
	gchar *didl_item = NULL;
	GtkTreeIter iter;

	TRACE(( "get_browser_item( \"%s\" )\n", path_str ));

	if ( gtk_tree_model_get_iter( model, &iter, path ) )
	{
	#ifdef PIMP_MY_BROWSER

		const char *browser_item;
		struct xml_info info;

		ASSERT( browser_items != NULL && path_str != NULL );
		if ( browser_items == NULL || path_str == NULL )
		{
			GError *error = NULL;
			g_set_error( &error, 0, 1, Text(ERRMSG_INTERNAL) );
			HandleError( error, Text(ERROR_HANDLING_ITEM), "?" );
			return NULL;
		}

		browser_item = browser_items[atoi( path_str )];
		ASSERT( browser_item != NULL );
		if ( browser_item == NULL )
		{
			GError *error = NULL;
			g_set_error( &error, 0, 2, Text(ERRMSG_INTERNAL) );
			HandleError( error, Text(ERROR_HANDLING_ITEM), "?" );
			return NULL;
		}

		if ( xml_get_info( browser_item, &info ) && info.next != NULL )
		{
			didl_item = g_strndup( browser_item, info.next - browser_item );
		}
		else
		{
			didl_item = g_strdup( browser_item );
		}

	#else

		gtk_tree_model_get( model, &iter, COL_ITEM, &didl_item, -1 );

	#endif
	}

	return didl_item;
}

gboolean browser_action( int action, gchar **didl_item, gchar **path_str, gfloat row_align )
{
	gboolean success = FALSE;

	set_saved_item( didl_item, path_str, row_align );

	switch ( action )
	{
	case BROWSER_POPUP_MENU:          /* Popup menu */
		show_popup_menu();
		success = TRUE;
		break;

	case BROWSER_OPEN:                /* Open */
		success = server_or_container_open();
		break;

	case BROWSER_REPLACE_PLAYLIST:    /* Replace playlist */
	case BROWSER_SET_CHANNEL:         /* Set radio channel */
		success = container_or_item_replace();
		break;

	case BROWSER_APPEND_TO_PLAYLIST:  /* Append to playlist */
		if ( IsLastfmTrack )
		{
			gchar *title = GetTitle( saved_item.didl, NULL, 0 );
			GError *error = NULL;

			g_set_error( &error, 0, 0, Text(ERRMSG_LASTFM_RADIO) );
			HandleError( error, Text(ERROR_PLAYLIST_ADD), title );
			g_free( title );
		}
		else
		{
			success = container_or_item_append();
		}
		break;

	case BROWSER_PLAY_LASTFM:
		success = play_lastfm( saved_item.didl );
		free_saved_item();
		break;
	}

	return success;
}

/*-----------------------------------------------------------------------------------------------*/

static void browse_server_or_container( const char *id, const char *title )
{
	char dtostr_buf[G_ASCII_DTOSTR_BUF_SIZE];

	TRACE(( "-> browse_server_or_container( id = \"%s\", title = \"%s\" )\n", id, title ));

	append_to_path( id_path, id );
	append_to_path( title_path, title );
	replace_current_path( tree_path, saved_item.path );
	append_to_path( tree_path, "-" );  /* no selection */
	replace_current_path( align_path, g_ascii_dtostr( dtostr_buf, G_ASCII_DTOSTR_BUF_SIZE, saved_item.row_align ) );
	append_to_path( align_path, g_ascii_dtostr( dtostr_buf, G_ASCII_DTOSTR_BUF_SIZE, 0.0 ) );

	start_browsing( TRUE );

	TRACE(( "<- browse_server_or_container()\n" ));
}

gboolean server_or_container_open( void /*GtkMenuItem *menu_item, gpointer user_data*/ )
{
	const char *class_id = get_upnp_class_id( saved_item.didl );
	gboolean success = FALSE;
	GError *error = NULL;

	TRACE(( "## server_or_container_open()\n" ));

	busy_enter();

	if ( class_id == mediaServerId )
	{
		upnp_device *device = get_server( saved_item.didl );
		if ( device != NULL )
		{
			set_browser_device( device );
			browse_server_or_container( "0", device->friendlyName );
			success = TRUE;
		}
		else
		{
			g_set_error( &error, 0, 1, Text(ERRMSG_INTERNAL) );
		}
	}
	else if ( class_id == localId )
	{
		browse_server_or_container( saved_item.didl, get_local_title() );
		success = TRUE;
	}
	else if ( class_id == radioId )
	{
		browse_server_or_container( saved_item.didl, get_radio_title() );
		success = TRUE;
	}
	else if ( class_id == radiotimeId )
	{
		browse_server_or_container( saved_item.didl, GetRadioTimeTitle() );
		success = TRUE;
	}
	else if ( class_id == lastfmId )
	{
		browse_server_or_container( saved_item.didl, GetLastfmRadioTitle() );
		success = TRUE;
	}
	else if ( class_id == containerId || class_id == playlistContainerId || class_id == musicAlbumId || class_id == localFolderId )
	{
		gchar *id, *title;

		id = xml_get_named_attribute_str( saved_item.didl, "id" );
		if ( id != NULL )
		{
			title = xml_get_named_str( saved_item.didl, "dc:title" );
			if ( title != NULL )
			{
				browse_server_or_container( id, title );
				success = TRUE;
				g_free( title );
			}

			g_free( id );
		}
	}
	else
	{
		g_set_error( &error, 0, 2, Text(ERRMSG_INTERNAL) );
	}

	busy_leave();

	if ( error != NULL )
	{
		gchar *title = GetTitle( saved_item.didl, NULL, 0 );
		HandleError( error, Text(ERROR_BROWSE_FOLDER), title );
		g_free( title );
	}

	free_saved_item();
	return success;
}

#define ALBUM_ART_SIZE   200
#define MAX_ALBUM_INFO_1  36  /* g_utf8_strlen( "Joanna Newsom and the Ys Street Band" ) */
#define MAX_ALBUM_INFO_0  54

static int append_album_info( GtkListStore *store, const char *name, const gchar *didl, const char *xml_name, int max_info )
{
	gchar *value = xml_get_named_str( didl, xml_name );
	int result = 0;

	ASSERT( store != NULL && name != NULL );

	if ( value != NULL )
	{
		if ( AppendInfo( store, name, value, -max_info ) )
			result++;

		g_free( value );
	}

	return result;
}

static gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, GError *error )
{
	if ( error != NULL )
	{
		HandleError( g_error_copy( error ), Text(ERROR_GET_ALBUM_COVER) );
		return TRUE;
	}

	return FALSE;
}

static void album_info_dialog( const gchar *didl, GtkImage *image, GError *error )
{
	GtkDialog *info;
	gchar *value;

	GtkTreeView *view = NULL;
	GtkListStore *store;
	int max_info, n = 0;

	TRACE(( "album_info_dialog( \"%s\" )\n", didl ));

	value = GetTitle( didl, NULL, 0 );
	info = GTK_DIALOG(gtk_dialog_new_with_buttons( value,
		GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL ));
	g_free( value );

	gtk_dialog_set_has_separator( info, FALSE );
	gtk_dialog_set_default_response( info, GTK_RESPONSE_OK );

	store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_STRING );

/*TRACE(( "DIDL-Item: %s\n", didl ));*/
	max_info = (image == NULL) ? MAX_ALBUM_INFO_0 : MAX_ALBUM_INFO_1;

	/* append_album_info( store, Text(DC_TITLE), didl, "dc:title", max_info );*/
	n += append_album_info( store, Text(UPNP_ARTIST), didl, "upnp:artist", max_info );
	append_album_info( store, Text(DC_CREATOR), didl, "dc:creator", max_info );
	if ( get_upnp_class_id( didl ) != musicAlbumId )
		n += append_album_info( store, Text(UPNP_ALBUM), didl, "upnp:album", max_info );
	n += append_album_info( store, Text(DC_DATE), didl, "dc:date", max_info );
	n += append_album_info( store, Text(UPNP_GENRE), didl, "upnp:genre", max_info );

	n += append_album_info( store, Text(RADIOTIME_SUBTEXT), didl, "radiotime:subtext", max_info );
	n += append_album_info( store, Text(RADIOTIME_BITRATE), didl, "radiotime:bitrate", max_info );
	n += append_album_info( store, Text(RADIOTIME_RELIABILITY), didl, "radiotime:reliability", max_info );
	n += append_album_info( store, Text(RADIOTIME_CURRENT_TRACK), didl, "radiotime:current_track", max_info );

	value = GetDuration( didl, NULL, 0 );
	if ( value != NULL )
	{
	#if LOGLEVEL > 0
		struct xml_info info;
		const char *res = xml_find_named( didl, "res", &info );
		gchar *duration = xml_get_named_attribute_str( res, "duration" );
		gchar *extended_value = g_strdup_printf( "%s (%s)", value, duration );

		if ( AppendInfo( store, Text(RES_DURATION), extended_value, max_info ) ) n++;

		g_free( extended_value );
		g_free( duration );
	#else
		if ( AppendInfo( store, Text(RES_DURATION), value, max_info ) ) n++;
	#endif

		g_free( value );
	}

	if ( n > 0 )
	{
		GtkTreeSelection *selection;
		GtkTreeViewColumn *column;
		GtkCellRenderer *renderer;

		view = GTK_TREE_VIEW( gtk_tree_view_new_with_model( GTK_TREE_MODEL( store ) ) );
		gtk_tree_view_set_enable_search( view, FALSE );
		gtk_tree_view_set_headers_visible( view, FALSE );
		g_object_set( view, "can-focus", (gboolean)FALSE, NULL );
		g_object_unref( store );

		selection = gtk_tree_view_get_selection( view );
		gtk_tree_selection_set_mode( selection, GTK_SELECTION_EXTENDED /* ?? */ );

		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start( column, renderer, FALSE );
		gtk_tree_view_column_set_attributes( column, renderer, "text", 0, NULL );
		gtk_tree_view_append_column( view, column );

		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start( column, renderer, FALSE );
		gtk_tree_view_column_set_attributes( column, renderer, "text", 1, NULL );
		gtk_tree_view_append_column( view, column );

		gtk_tree_selection_unselect_all( selection );
	}
	else
	{
		g_object_unref( store );
	}

	if ( image != NULL && view != NULL )
	{
		gint content_area_border = 3;
		GtkEventBox *event_box;
		GtkBox *hbox;

		g_object_get( info, "content-area-border", &content_area_border, NULL );
		TRACE(( "content-area-border = %d\n", content_area_border ));

		hbox = GTK_BOX( gtk_hbox_new( FALSE, content_area_border ) );
		event_box = GTK_EVENT_BOX( gtk_event_box_new() );

		g_signal_connect( G_OBJECT(event_box), "button-release-event", G_CALLBACK(button_release_event), error );
		gtk_container_add( GTK_CONTAINER(event_box), GTK_WIDGET(image) );

		gtk_box_pack_start( hbox, GTK_WIDGET(event_box), FALSE, FALSE, 0 );
		gtk_box_pack_start( hbox, GTK_WIDGET(view), FALSE, FALSE, 0 );

		gtk_box_pack_start_defaults( GTK_BOX(info->vbox), GTK_WIDGET(hbox) );
	}
	else if ( image != NULL )
	{
		gtk_box_pack_start_defaults( GTK_BOX(info->vbox), GTK_WIDGET(image) );
	}
	else if ( view != NULL )
	{
		gtk_box_pack_start_defaults( GTK_BOX(info->vbox), GTK_WIDGET(view) );
	}
	else
	{
		ASSERT( FALSE );
	}

	gtk_widget_show_all( GTK_WIDGET(info) );
	gtk_dialog_run( info );
	gtk_widget_destroy( GTK_WIDGET(info) );
}

struct album_info_struct
{
	gchar *didl, *albumArtURI;
	char *content, *content_type;
	size_t content_length;
	gboolean ok;
	GError *error;
};

gboolean album_info_thread_end( gpointer data )
{
	struct album_info_struct *ais = (struct album_info_struct *)data;
	GtkImage *image;

	gdk_threads_enter();

	image = g_object_new( GTK_TYPE_IMAGE, NULL );
	if ( ais->ok )
	{
		/* Show image */
		SetAlbumArt( image, ALBUM_ART_SIZE, TRUE,
			ais->albumArtURI, ais->content_type, ais->content, ais->content_length,
			&ais->error );

		upnp_free_string( ais->content );
	}
	else if ( ais->error != NULL )
	{
		/* Show "missing image" */
		SetAlbumArt( image, ALBUM_ART_SIZE, TRUE, NULL, NULL, NULL, 0, NULL );
	}
	else
	{
		/* No image (should not happen here) */
		ASSERT( FALSE );
		g_object_unref( image );
		image = NULL;
	}

	busy_leave();

	album_info_dialog( ais->didl, image, ais->error );

	if ( ais->error != NULL ) g_error_free( ais->error );
	g_free( ais->albumArtURI );
	g_free( ais->didl );
	g_free( ais );

	gdk_threads_leave();
	return FALSE;
}

static gpointer album_info_thread( gpointer data )
{
	struct album_info_struct *ais = (struct album_info_struct *)data;

	ais->ok = GetAlbumArt( ais->albumArtURI,
		&ais->content_type, &ais->content, &ais->content_length,
		&ais->error );

	g_idle_add( album_info_thread_end, ais );
	return NULL;
}

gboolean server_info( void /*GtkMenuItem *menu_item, gpointer user_data*/ )
{
	upnp_device *device = get_server( saved_item.didl );
	if ( device != NULL ) Info( device );

	free_saved_item();
	return TRUE;
}

gboolean container_or_item_info( void /*GtkMenuItem *menu_item, gpointer user_data*/ )
{
	gchar *albumArtURI;

	TRACE(( "## container_or_item_info()\n" ));

	albumArtURI = GetInfo( saved_item.didl, "upnp:albumArtURI", NULL, 0 );
	if ( albumArtURI != NULL )
	{
		struct album_info_struct *ais = g_new0( struct album_info_struct, 1 );

		ais->didl = saved_item.didl; saved_item.didl = NULL;
		ais->albumArtURI = albumArtURI;

		busy_enter();
		TRACE(( "album_info_thread( \"%s\" )\n", albumArtURI ));
		g_thread_create( album_info_thread, ais, FALSE, NULL );
	}
	else
	{
		album_info_dialog( saved_item.didl, NULL, NULL );
	}

	free_saved_item();
	return TRUE;
}

gboolean container_or_item_replace( void /*GtkMenuItem *menu_item, gpointer user_data*/ )
{
	gboolean result = FALSE;

	TRACE(( "## container_or_item_replace()\n" ));

	if ( IsLastfmItem( saved_item.didl ) )
	{
		result = play_lastfm( saved_item.didl );
	}
	else if ( ready_to_replace() )
	{
		busy_enter();

		if ( ClearPlaylist() )
		{
			ASSERT( !IsLastfmTrack );
			if ( IsLastfmTrack )
			{
				GError *error = NULL;
				g_set_error( &error, 0, 42, Text(ERRMSG_INTERNAL) );
				HandleError( error, Text(ERROR_PLAYLIST_CLEAR) );
			}
			else if ( container_or_item_append() )
			{
				ClearNowPlaying( FALSE );
				SetCurrentPage( Settings->browser.page_after_replacing_playlist );
				result = TRUE;
			}
		}

		busy_leave();
	}

	free_saved_item();
	return result;
}

gboolean container_or_item_append( void /*GtkMenuItem *menu_item, gpointer user_data*/ )
{
	gboolean success = FALSE;
	gchar *upnp_class;

	TRACE(( "## container_or_item_append()\n" ));

	if ( IsLastfmTrack ) return container_or_item_replace();

	busy_enter();

	upnp_class = xml_get_named_str( saved_item.didl, "upnp:class" );
	if ( upnp_class != NULL )
	{
		TRACE(( "container_or_item_append(): upnp_class = \"%s\"\n", upnp_class ));

		if ( strncmp( upnp_class, "object.container", 16 ) == 0 )
		{
			gchar *id = xml_get_named_attribute_str( saved_item.didl, "id" );

			if ( id != NULL )
			{
				gchar *title_and_artist = GetTitleAndArtist( saved_item.didl, NULL, 0 );
				struct xml_content *result;
				GError *error = NULL;

				TRACE(( "album_append(): id = %s\n", id ));

				result = browse_children( id, "upnp:originalTrackNumber", &error );
				if ( result != NULL )
				{
					success = AddToPlaylist( result->str, title_and_artist );
					free_xml_content( result );
				}
				else
				{
					HandleError( error, Text(ERROR_BROWSE_ALBUM), title_and_artist );
				}

				g_free( title_and_artist );
				g_free( id );
			}
		}
		else if ( strncmp( upnp_class, "object.item.audioItem", 21 ) == 0 )
		{
			if ( IsLastfmItem( saved_item.didl ) )
			{
				success = play_lastfm( saved_item.didl );
			}
			else
			{
				gchar *title_and_artist = GetTitleAndArtist( saved_item.didl, NULL, 0 );

				success = AddToPlaylist( saved_item.didl, title_and_artist );
				g_free( title_and_artist );
			}
		}
		else
		{
			TRACE(( "*** container_or_item_append(): Item with upnp:class %s ignored", upnp_class ));
		}
	}

	g_free( upnp_class );

	busy_leave();

	free_saved_item();
	return success;
}

gboolean ready_to_replace( void )
{
	gint response = IsStopped()
		? GTK_RESPONSE_YES
		: message_dialog( GTK_MESSAGE_QUESTION, Text(BROWSER_REPLACE_PLAYLIST_QUESTION) );

	return ( response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES );
}

/*-----------------------------------------------------------------------------------------------*/

const char *get_current_path( GString *path )
{
	if ( path != NULL )
	{
		const char *s = strrchr( path->str, PATH_SEPARATOR );
		return ( s != NULL ) ? s + 1 : path->str;
	}

	return NULL;
}

void append_to_path( GString *path, const gchar *append )
{
	g_string_append_c( path, PATH_SEPARATOR );
	g_string_append( path, append );
}

void replace_current_path( GString *path, const gchar *replace )
{
	const char *s = get_current_path( path );
	g_string_truncate( path, s - path->str );
	g_string_append( path, replace );
}

gboolean go_back_in_path( GString *path )
{
	const char *s = strrchr( path->str, PATH_SEPARATOR );
	if ( s == NULL ) return FALSE;
	g_string_truncate( path, s - path->str );
	return TRUE;
}

gboolean can_go_back_in_path( void /*GString *path*/ )
{
	GString *path = id_path;
	return path != NULL && strchr( path->str, PATH_SEPARATOR ) != NULL;
}

/*-----------------------------------------------------------------------------------------------*/

const char *get_device_id( const char *object_id )
{
	const char *device_id;

	if ( is_local_folder( object_id ) )
		device_id = local_id;
	else if ( is_radiotime_folder( object_id ) )
		device_id = radiotime_id;
	else if ( browser_device != NULL )
		device_id = browser_device->UDN;
	else
		device_id = NULL;

	return device_id;
}

struct browser_cache_entry
{
	struct browser_cache_entry *next;

	const char *device_id;
	gchar *object_id;
	const char *sort_criteria;
	struct xml_content *result;
} *browser_cache;

static struct browser_cache_entry *get_browser_cache_entry( const char *device_id, const char *object_id, const char *sort_criteria )
{
	struct browser_cache_entry *entry;

	for ( entry = browser_cache; entry != NULL; entry = entry->next )
	{
		if ( entry->device_id == device_id && strcmp( entry->object_id, object_id ) == 0 &&
		     (sort_criteria == NULL || strcmp( entry->sort_criteria, sort_criteria ) == 0) )
		{
			TRACE(( "get_browser_cache_entry() = %p\n", (void*)entry ));
			return entry;
		}
	}

	TRACE(( "get_browser_cache_entry() = NULL\n" ));
	return NULL;
}

static gboolean free_browser_cache_entry( struct browser_cache_entry *entry, gboolean remove_from_list )
{
	if ( remove_from_list )
	{
		if ( !list_remove( &browser_cache, entry ) )
		{
			TRACE(( "******* free_browser_cache_entry(): list_remove() failed\n" ));
			return FALSE;
		}
	}

	free_xml_content( entry->result );
	g_free( entry->object_id );
	g_free( entry );

	return TRUE;
}

struct xml_content *get_from_browser_cache( const char *device_id, const char *object_id, const char *sort_criteria )
{
	struct browser_cache_entry *entry = get_browser_cache_entry( device_id, object_id, sort_criteria );
	if ( entry != NULL )
	{
		if ( entry->next != NULL )
		{
			list_remove( &browser_cache, entry );
			list_append( &browser_cache, entry );
		}

		TRACE(( "get_from_browser_cache( \"%s\", \"%s\", \"%s\" ) = %p\n", device_id, object_id, sort_criteria, (void*)&entry->result ));
		return copy_xml_content( entry->result );
	}

	TRACE(( "get_from_browser_cache( \"%s\", \"%s\", \"%s\" ) = NULL\n", device_id, object_id, sort_criteria ));
	return NULL;
}

void remove_from_browser_cache( const char *device_id, const char *object_id )
{
	for (;;)
	{
		struct browser_cache_entry *entry = get_browser_cache_entry( device_id, object_id, NULL );
		if ( entry == NULL ) break;

		free_browser_cache_entry( entry, TRUE );
	}
}

void put_into_browser_cache( const char *device_id, const char *object_id, const char *sort_criteria, struct xml_content *result )
{
	if ( BROWSER_CACHE_ENTRIES > 0 )
	{
		struct browser_cache_entry *entry = g_new( struct browser_cache_entry, 1 );

		TRACE(( "put_into_browser_cache( \"%s\", \"%s\", %p )\n", device_id, object_id, (void*)result ));
		ASSERT( result != NULL && result->str != NULL );

		entry->device_id = device_id;
		entry->object_id = g_strdup( object_id );
		entry->sort_criteria = sort_criteria;
		entry->result = copy_xml_content( result );

		if ( list_append( &browser_cache, entry ) >= BROWSER_CACHE_ENTRIES )
		{
			/* remove oldest entry */
			TRACE(( "put_into_browser_cache(): removing oldest entry\n" ));
			free_browser_cache_entry( browser_cache, TRUE );
		}
	}
}

void clear_browser_cache( const char *device_id )
{
	struct browser_cache_entry *entry;

	TRACE(( "clear_browser_cache( \"%s\" )\n", (device_id != NULL) ? device_id : "<NULL>" ));

	if ( device_id == NULL )
	{
		entry = browser_cache;
		browser_cache = NULL;
		while ( entry != NULL )
		{
			struct browser_cache_entry *next = entry->next;
			free_browser_cache_entry( entry, FALSE );
			entry = next;
		}
	}
	else
	{
		for ( entry = browser_cache; entry != NULL; )
		{
			if ( entry->device_id == device_id )
			{
				if ( !free_browser_cache_entry( entry, TRUE ) ) break;
				entry = browser_cache;
			}
			else
			{
				entry = entry->next;
			}
		}
	}
}

/*-----------------------------------------------------------------------------------------------*/
#ifndef MAEMO

static gboolean drag_begin_func( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data )
{
	GtkWidget *widget = (GtkWidget *)user_data;

	gchar *item = get_browser_item( tree_view, path, path_str );
	if ( item != NULL )
	{
		const char *class_id = get_upnp_class_id( item );

		if ( !is_dragable_item( item, class_id ) ) class_id = GTK_STOCK_NO; /* or GTK_STOCK_CANCEL */
		gtk_drag_source_set_icon_stock( widget, class_id );

		g_free( item );
		return FALSE;
	}

	return TRUE;
}

void browser_drag_begin( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data )
{
	TRACE(( "browser_drag_begin()\n" ));
	foreach_selected_tree_view_row( browser, drag_begin_func, widget );
}

static gboolean drag_data_get_func( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data )
{
	GString *data = (GString *)user_data;

	gchar *item = get_browser_item( tree_view, path, path_str );
	if ( item != NULL )
	{
		TRACE(( "drag_data_get_func(): Füge Eintrag #%s hinzu...\n", path_str ));
		g_string_append( data, item );
		g_free( item );
	}

	return TRUE;
}

void browser_drag_data_get( GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data )
{
	GString *data = g_string_new( "" );

	TRACE(( "browser_drag_data_get()\n" ));
	foreach_selected_tree_view_row( browser, drag_data_get_func, data );
	gtk_selection_data_set_text( selection_data, data->str, data->len );

	g_string_free( data, TRUE );
}

/*gboolean browser_drag_failed( GtkWidget *widget, GdkDragContext *drag_context, GtkDragResult result, gpointer user_data )
{
	TRACE(( "browser_drag_failed()\n" ));
	return FALSE;
}*/

void browser_drag_end( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data )
{
	TRACE(( "browser_drag_end()\n" ));
	;
}

#endif
/*-----------------------------------------------------------------------------------------------*/

