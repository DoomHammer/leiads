/*
	now_playing.c
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

#define CURRENT_SMALL_BORDER_WIDTH  24
#define CURRENT_LARGE_BORDER_WIDTH  48
#define CURRENT_ROW_SPACINGS        12
#define CURRENT_COL_SPACINGS        18

#define EOP_ICON_STOCK_ID   GTK_STOCK_MEDIA_STOP
#define EOP_ICON_SIZE       GTK_ICON_SIZE_SMALL_TOOLBAR
#define EOP_ICON_WIDTH      18  /* see "The official GNOME2 Developer's Guide", page 137 */

#define ASSUMED_LABEL_SIZE  300

/*-----------------------------------------------------------------------------------------------*/

int current_border_width;
GtkBox *current_hbox;
GtkImage *current_image, *current_radio;

enum radio_image { NO_IMAGE = 0, LASTFM_IMAGE, RADIOTIME_IMAGE } current_radio_image;
GtkLabel *current_label;
GError *current_error;

gchar *current_album;
unsigned current_image_id;

struct image_params
{
	gchar *uri;
	unsigned id;
};

void show_radio_popup_menu( void );

void set_current_album( gchar *album );
void set_current_radio_image( enum radio_image image );

gpointer load_current_image( gpointer data );
void clear_current_image( void );

void set_current_box_spacing( gboolean album_art );

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
static gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );

/*-----------------------------------------------------------------------------------------------*/

G_BEGIN_DECLS

#define MY_TYPE_EVENT_BOX (my_event_box_get_type())
#define MY_EVENT_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST( (obj), MY_TYPE_EVENT_BOX, MyEventBox ))

typedef struct _MyEventBox
{
	GtkEventBox event_box;
} MyEventBox;

typedef struct _MyEventBoxClass
{
	GtkEventBoxClass parent_class;
} MyEventBoxClass;

GType my_event_box_get_type( void ) G_GNUC_CONST;
GtkWidget *my_event_box_new( void );

G_END_DECLS

static void my_event_box_realize( GtkWidget *widget );

G_DEFINE_TYPE( MyEventBox, my_event_box, GTK_TYPE_EVENT_BOX )

static void my_event_box_class_init( MyEventBoxClass *cls )
{
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS( cls );
	widget_class->realize = my_event_box_realize;
}

static void my_event_box_init( MyEventBox *event_box )
{
	;  /* nothing to do */
}

GtkWidget *my_event_box_new( void )
{
	return g_object_new( MY_TYPE_EVENT_BOX, NULL );
}

static void my_event_box_realize( GtkWidget *widget )
{
	guint8 state;

	(*GTK_WIDGET_CLASS( my_event_box_parent_class )->realize)( widget );

	ASSERT( !GTK_WIDGET_NO_WINDOW( widget ) );
	state = GTK_WIDGET_STATE( widget );
	gtk_widget_modify_bg( widget, state, get_widget_base_color( widget, state ) );
}

/*-----------------------------------------------------------------------------------------------*/

void CreateNowPlayingToolItems( GtkToolbar *toolbar, gint icon_index )
{
	;  /* nothing to do */
}

void CreateNowPlayingMenuItems( GtkMenuShell *menu )
{
	;  /* nothing to do */
}

/*-----------------------------------------------------------------------------------------------*/

GtkWidget *CreateNowPlaying( void )
{
	GtkBox *hbox, *outer_vbox, *vbox, *radio_hbox;
	MyEventBox *event_box;

	TRACE(( "-> CreateNowPlaying()\n" ));

	current_radio = GTK_IMAGE( gtk_image_new() );
	current_radio_image = NO_IMAGE;

	radio_hbox = GTK_BOX( gtk_hbox_new( FALSE, 0 ) );
	gtk_box_pack_end( radio_hbox, GTK_WIDGET(current_radio), FALSE, FALSE, 0 );

	current_image = g_object_new( GTK_TYPE_IMAGE, NULL );
	ASSERT( current_image != NULL );
	gtk_misc_set_alignment( GTK_MISC(current_image), 1.0, 1.0 );  /* right & bottom aligned */

	current_label = GTK_LABEL( gtk_label_new( "" ) );
	ASSERT( current_label != NULL );
	gtk_misc_set_alignment( GTK_MISC(current_label), 0.0, 1.0 );  /* left & bottom aligned */
	gtk_label_set_line_wrap( current_label, TRUE );
	gtk_label_set_line_wrap_mode( current_label, PANGO_WRAP_WORD_CHAR );
#ifndef MAEMO
	gtk_label_set_ellipsize( current_label, PANGO_ELLIPSIZE_END );
#endif

	hbox = GTK_BOX( gtk_hbox_new( FALSE, CURRENT_COL_SPACINGS ) );
	ASSERT( hbox != NULL );
	current_border_width = ( Settings->MID || !Settings->startup.classic_ui ) ? CURRENT_SMALL_BORDER_WIDTH : CURRENT_LARGE_BORDER_WIDTH;
	gtk_container_set_border_width( GTK_CONTAINER(hbox), current_border_width );

	gtk_box_pack_start( hbox, GTK_WIDGET(current_image), FALSE, FALSE, 0 );

	vbox = GTK_BOX( gtk_vbox_new( FALSE, CURRENT_ROW_SPACINGS ) );
	ASSERT( vbox != NULL );
	gtk_box_pack_start( vbox, GTK_WIDGET(radio_hbox), FALSE, FALSE, 0 );
	gtk_box_pack_end( vbox, GTK_WIDGET(current_label), TRUE, TRUE, 0 );
	gtk_box_pack_start( hbox, GTK_WIDGET(vbox), TRUE, TRUE, 0 );

	outer_vbox = GTK_BOX( gtk_vbox_new( FALSE, 0 ) );
	ASSERT( outer_vbox != NULL );
	gtk_box_pack_start( outer_vbox, GTK_WIDGET(hbox), TRUE, FALSE, 0 );

	event_box = MY_EVENT_BOX( my_event_box_new() );
	ASSERT( event_box != NULL );
#if 0
	{
		GdkColor color;
		gdk_color_parse( "#FFFFFF", &color );
		gtk_widget_modify_base( GTK_WIDGET(event_box), GTK_STATE_NORMAL, &color );
	}
#endif
	g_signal_connect( G_OBJECT(event_box), "button-press-event", G_CALLBACK(button_press_event), NULL );
	g_signal_connect( G_OBJECT(event_box), "button-release-event", G_CALLBACK(button_release_event), NULL );
	gtk_container_add( GTK_CONTAINER(event_box), GTK_WIDGET(outer_vbox) );

	g_signal_connect( G_OBJECT(event_box), "key-press-event", G_CALLBACK(KeyPressEvent), GINT_TO_POINTER(NOW_PLAYING_PAGE) );
	g_signal_connect( G_OBJECT(event_box), "key-release-event", G_CALLBACK(KeyReleaseEvent), GINT_TO_POINTER(NOW_PLAYING_PAGE) );

#ifndef MAEMO
	if ( !Settings->startup.classic_ui )
	{
		gtk_drag_dest_set( GTK_WIDGET(event_box), GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
			TargetList, G_N_ELEMENTS( TargetList ), GDK_ACTION_DEFAULT|GDK_ACTION_MOVE|GDK_ACTION_PRIVATE );

		/*g_signal_connect( G_OBJECT(event_box), "drag-drop", GTK_SIGNAL_FUNC(drag_drop), NULL );*/
		g_signal_connect( G_OBJECT(event_box), "drag-data-received", GTK_SIGNAL_FUNC(drag_data_received), GUINT_TO_POINTER(GDK_ACTION_MOVE) );
	}
#endif

	TRACE(( "<- CreateNowPlaying()\n" ));
	current_hbox = hbox;
	return GTK_WIDGET( event_box );
}

int OpenNowPlaying( void )
{
	TRACE(( "-> OpenNowPlaying()\n" ));

	ASSERT( current_album == NULL );

	TRACE(( "<- OpenNowPlaying()\n" ));
	return 0;
}

void CloseNowPlaying( void )
{
	TRACE(( "-> CloseNowPlaying()\n" ));

	ClearNowPlaying( FALSE );

	TRACE(( "<- CloseNowPlaying()\n" ));
}

void ClearNowPlaying( gboolean lastfm )
{
	set_current_album( NULL );
	set_current_radio_image( (lastfm) ? LASTFM_IMAGE : NO_IMAGE );

	clear_current_image();
	gtk_label_set_text( current_label, "" );
}

/*-----------------------------------------------------------------------------------------------*/

void OnSelectNowPlaying( void )
{
	TRACE(( "-> OnSelectNowPlaying()\n" ));

#if 0
#ifdef MAEMO
	if ( Settings->last_fm.active )
		gtk_widget_show( GTK_WIDGET(ti_lastfm) );
#else
	if ( Settings->last_fm.active )
	{
		gtk_widget_show( GTK_WIDGET(ti_lastfm) );
		gtk_widget_set_sensitive( GTK_WIDGET(ti_lastfm), TRUE );
	}
#endif
#endif

	TRACE(( "<- OnSelectNowPlaying()\n" ));
}

void OnUnselectNowPlaying( void )
{
	TRACE(( "-> OnUnselectNowPlaying()\n" ));

#if 0
#ifdef MAEMO
	gtk_widget_hide( GTK_WIDGET(ti_lastfm) );
#else
	if ( Settings->last_fm.active )
		gtk_widget_set_sensitive( GTK_WIDGET(ti_lastfm), FALSE );
	else
		gtk_widget_hide( GTK_WIDGET(ti_lastfm) );
#endif
#endif

	TRACE(( "<- OnUnselectNowPlaying()\n" ));
}

void OnNowPlayingKey( GdkEventKey *event, struct keybd_function **func )
{
	switch ( event->keyval )
	{
	case GDK_F5:
		OnPlaylistKey( event, func );
		break;
	}
}

void OnNowPlayingSettingsChanged( void )
{
	SetNowPlayingAttributes( -1, -1 );

	if ( CurrentTrackIndex >= 0 )
	{
		set_current_album( NULL );
		OnNowPlayingTrackIndex( CurrentTrackIndex, CurrentTrackIndex );
	}
}

/*-----------------------------------------------------------------------------------------------*/

void OnNowPlayingTrackIndex( int OldTrackIndex, int NewTrackIndex )
{
	enum radio_image image = NO_IMAGE;

	TRACE(( "-> OnNowPlayingTrackIndex( %d => %d )\n", OldTrackIndex, NewTrackIndex ));

	if ( NewTrackIndex >= 0 )
	{
		gchar *item = GetTrackItem( NewTrackIndex );

		if ( item != NULL )
		{
			GString *label = g_string_sized_new( 256 );
			gboolean album = FALSE;
			gchar *artist, *value;

			TRACE(( "ITEM: %s\n", item ));
			ASSERT( label != NULL );

			if ( Settings->now_playing.album_art_size != 0 &&
			     (value = GetAlbumArtURI( item )) != NULL )
			{
				gchar *album = xml_get_named_str( item, "upnp:album" );

				TRACE(( "albumArtURI = %s\n", value ));
				if ( current_album != NULL && album != NULL )
				{
					TRACE(( "old album = %s\n", current_album ));
					TRACE(( "new album = %s\n", album ));
					TRACE(( "strcmp() = %d\n", strcmp( current_album, album ) ));
				}

				if ( current_album == NULL || album == NULL || strcmp( current_album, album ) != 0 )
				{
					struct image_params *params = g_new( struct image_params, 1 );

					set_current_album( album );

					params->uri = value;
					params->id = ++current_image_id;

					if ( OldTrackIndex < 0 )
					{
						gtk_image_clear( current_image );
						set_current_box_spacing( FALSE );
					}

					g_thread_create( load_current_image, params, FALSE, NULL );
				}
				else
				{
					g_free( album );
					g_free( value );
				}
			}
			else
			{
				clear_current_image();
			}

			if ( Settings->now_playing.show_track_number && !IsLastfmTrack && (NewTrackIndex != 0 || GetTrackCount() > 1) )
			{
				g_string_append_printf( label, "%d.", 1 + NewTrackIndex );
			}

			if ( (value = GetTitle( item, NULL, 0 )) != NULL )
			{
				g_string_append_printf( label, "\n%s", value );
				g_free( value );
			}

			if ( Settings->now_playing.show_track_duration && (value = GetDuration( item, NULL, 0 )) != NULL )
			{
				g_string_append_printf( label, " (%s)", value );
				g_free( value );
			}

			if ( (artist = GetArtist( item, NULL, 0 )) != NULL )
			{
				g_string_append_printf( label, "\n%s", artist );
				/*g_free( artist );*/
			}

			if ( (value = GetAlbum( item, NULL, 0 )) != NULL )
			{
				g_string_append_printf( label, "\n\n%s", value );
				g_free( value );

				if ( artist != NULL && (value = GetAlbumArtist( item, NULL, 0 )) != NULL )
				{
					if ( strcmp( artist, value ) != 0 )
						g_string_append_printf( label, " (%s)", value );

					g_free( value );
				}

				album = TRUE;
			}

			g_free( artist );

			if ( Settings->now_playing.show_album_year )
			{
				if ( album )
				{
					if ( (value = GetYear( item, NULL, 0 )) != NULL )
					{
						g_string_append_printf( label, " (%s)", value );
						g_free( value );
					}
				}
				else
				{
					if ( (value = GetDate( item, NULL, 0 )) != NULL )
					{
						g_string_append_printf( label, "\n(%s)", value );
						g_free( value );
					}
				}
			}

			g_string_append_c( label, '\n' );

			gtk_label_set_text( current_label, label->str );
			g_string_free( label, TRUE );

			g_free( item );
		}
		else
		{
			clear_current_image();
			gtk_label_set_text( current_label, "?" );
		}

		if ( IsRadioTimeTrack ) image = RADIOTIME_IMAGE;
		else if ( IsLastfmTrack ) image = LASTFM_IMAGE;
	}
	else
	{
		clear_current_image();
		gtk_image_set_from_stock( current_image, EOP_ICON_STOCK_ID, EOP_ICON_SIZE );
		set_current_box_spacing( TRUE );
		gtk_label_set_text( current_label, Text(NOW_PLAYING_EOP) );
	}

	set_current_radio_image( image );

	TRACE(( "<- OnNowPlayingTrackIndex()\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

static void open_radio_page( GtkMenuItem* menu_item, const char *name )
{
	gchar *url = NULL;

	TRACE(( "open_lastfm_page( \"%s\" )\n", name ));

	if ( strcmp( name, "lastfm" ) == 0 )
	{
		url = g_strdup( "http://www.last.fm" );
	}
	else if ( strcmp( name, "radiotime" ) == 0 )
	{
		url = g_strdup( "http://radiotime.com" );
	}
	else
	{
		gchar *didl_item = GetCurrentTrackItem();
		if ( didl_item != NULL )
		{
			url = GetInfo( didl_item, name, NULL, 0 );
			g_free( didl_item );
		}
	}

	TRACE(( "open_radio_page(): URL = \"%s\"\n", (url != NULL) ? url : "<NULL>" ));
	if ( url != NULL )
	{
		ShowUri( GTK_WIDGET(current_radio), url );
		g_free( url );
	}
}

static int add_lastfm_popup_menu_item( GtkMenuShell *menu, const char *label, const char *didl_item, const char *name )
{
	char value[16];

	if ( IsValuableInfo( xml_get_named( didl_item, name, value, sizeof( value ) ) ) )
	{
		GtkWidget *item = gtk_menu_item_new_with_label( label );
		g_signal_connect( item, "activate", G_CALLBACK(open_radio_page), (gpointer)name );
		gtk_menu_shell_append( menu, item );

		return 1;
	}

	return 0;
}

void show_radio_popup_menu( void )
{
	gchar *didl_item = GetCurrentTrackItem();
	GtkMenuShell *menu;
	GtkWidget *item;
	int n;

	TRACE(( "-> show_radio_popup_menu()\n" ));

	menu = GTK_MENU_SHELL( gtk_menu_new() );

	if ( IsRadioTimeTrack )
	{
		item = gtk_menu_item_new_with_label( Text(RADIOTIME_HOME_PAGE) );
		g_signal_connect( item, "activate", G_CALLBACK(open_radio_page), "radiotime" );
		gtk_menu_shell_append( menu, item );
	}
	else if ( IsLastfmTrack )
	{
		n = 0;
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_ARTIST_PAGE), didl_item, "lastfm:artistpage" );
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_ALBUM_PAGE), didl_item, "lastfm:albumpage" );
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_TRACK_PAGE), didl_item, "lastfm:trackpage" );

		if ( n > 0 ) gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );

		n = 0;
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_BUY_TRACK_URL), didl_item, "lastfm:buyTrackURL" );
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_BUY_ALBUM_URL), didl_item, "lastfm:buyAlbumURL" );
		n += add_lastfm_popup_menu_item( menu, Text(LASTFM_FREE_TRACK_URL), didl_item, "lastfm:freeTrackURL" );

		if ( n > 0 ) gtk_menu_shell_append( menu, gtk_separator_menu_item_new() );

		item = gtk_menu_item_new_with_label( Text(LASTFM_HOME_PAGE) );
		g_signal_connect( item, "activate", G_CALLBACK(open_radio_page), "lastfm" );
		gtk_menu_shell_append( menu, item );
	}

	g_free( didl_item );

	gtk_menu_attach_to_widget( GTK_MENU(menu), GTK_WIDGET(current_radio), NULL );
	gtk_widget_show_all( GTK_WIDGET(menu) );

	gtk_menu_popup( GTK_MENU(menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time() );

	TRACE(( "<- show_radio_popup_menu()\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

void set_current_album( gchar *album )
{
	g_free( current_album );
	current_album = album;
}

void set_current_radio_image( enum radio_image image )
{
	gchar *filename = NULL;

	if ( image == NO_IMAGE )
	{
		if ( current_radio_image != NO_IMAGE )
		{
			current_radio_image = NO_IMAGE;
			gtk_image_clear( current_radio );
		}
	}
	else if ( image == LASTFM_IMAGE )
	{
		if ( current_radio_image != LASTFM_IMAGE )
		{
			extern GtkTreeView *browser;  /* a widget which was already "realized" */
			const GdkColor *bg = get_widget_base_color( GTK_WIDGET(browser), GTK_STATE_NORMAL );

			TRACE(( "set_current_radio_image(): Load Last.fm image...\n" ));
			current_radio_image = LASTFM_IMAGE;

			TRACE(( "set_current_radio_image(): bg = { %u,%u,%u }\n", bg->red, bg->green, bg->blue ));
			filename = GetLastfmLogoFilename( TRUE, TRUE, bg );
		}
	}
	else if ( image == RADIOTIME_IMAGE )
	{
		if ( current_radio_image != RADIOTIME_IMAGE )
		{
			TRACE(( "set_current_lastfm_image(): Load RadioTime image...\n" ));
			current_radio_image = RADIOTIME_IMAGE;

			filename = GetRadioTimeLogoFilename();
		}
	}
	else
	{
		ASSERT( FALSE );
	}

	if ( filename != NULL )
	{
		gchar *full_filename;

		TRACE(( "set_current_radio_image(): filename = \"%s\"\n", filename ));

		full_filename = BuildApplDataFilename( filename );
		gtk_image_set_from_file( current_radio, full_filename );
		g_free( full_filename );

		g_free( filename );
	}
}

/*-----------------------------------------------------------------------------------------------*/

gpointer load_current_image( gpointer data )
{
	struct image_params *params = (struct image_params *)data;

	char *content, *content_type;
	size_t content_length;
	gboolean ok;

	TRACE(( "load_current_image( \"%s\", %u )\n", params->uri, params->id ));

	if ( current_error != NULL )
	{
		g_error_free( current_error );
		current_error = NULL;
	}

	ok = GetAlbumArt( params->uri, &content_type, &content, &content_length, &current_error );

	gdk_threads_enter();

	TRACE(( "load_current_image(): GetAlbumArt( \"%s\" ) = %s\n", params->uri, (ok) ? "TRUE" : "FALSE" ));

	if ( params->id == current_image_id )  /* do we still need that image? */
	{
		if ( ok )
		{
			/* Show image */
			SetAlbumArt( current_image,
				Settings->now_playing.album_art_size, Settings->now_playing.shrink_album_art_only,
				params->uri, content_type, content, content_length, &current_error );
			set_current_box_spacing( TRUE );

			upnp_free_string( content );
		}
		else if ( current_error != NULL )
		{
			/* Show "missing image" */
			SetAlbumArt( current_image,
				Settings->now_playing.album_art_size, Settings->now_playing.shrink_album_art_only,
				NULL, NULL, NULL, 0, NULL );
			set_current_box_spacing( TRUE );
		}
		else
		{
			/* Clear image */
			gtk_image_clear( current_image );
			set_current_box_spacing( FALSE );
		}
	}
	else if ( ok )
	{
		TRACE(( "load_current_image(): discarding image data\n" ));
		upnp_free_string( content );
	}

	gdk_threads_leave();

	g_free( params->uri );
	g_free( params );
	return NULL;
}

void clear_current_image( void )
{
	TRACE(( "clear_current_image()\n" ));

	set_current_album( NULL );

	current_image_id++;
	gtk_image_clear( current_image );
	set_current_box_spacing( FALSE );

	if ( current_error != NULL )
	{
		g_error_free( current_error );
		current_error = NULL;
	}
}

/*-----------------------------------------------------------------------------------------------*/

void set_current_box_spacing( gboolean album_art )
{
	static int current_album_art = -1;

	TRACE(( "set_current_box_spacing( album_art = %s ): current_album_art = %d\n", (album_art) ? "TRUE" : "FALSE", current_album_art ));

	if ( album_art != current_album_art )
	{
		current_album_art = album_art;

		if ( album_art )
		{
			SetNowPlayingAttributes( -1, TRUE );
			gtk_box_set_spacing( current_hbox, CURRENT_COL_SPACINGS );
		}
		else
		{
			gtk_box_set_spacing( current_hbox, 0 );
			SetNowPlayingAttributes( -1, FALSE );
		}
	}
}

/*-----------------------------------------------------------------------------------------------*/

static gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	TRACE(( "# button_press_event( %d )\n", event->type ));

	if ( event->type == GDK_2BUTTON_PRESS && Settings->now_playing.double_click > 0 )
	{
		struct keybd_function *func = &keybd_functions[Settings->now_playing.double_click];
		(*func->func)( 0, FALSE );
		return TRUE;
	}

	return FALSE;
}

static gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	gint x, y;

	TRACE(( "# button_release_event( %d )\n", event->type ));

	if ( current_error != NULL )
	{
		widget = GTK_WIDGET(current_image);
		gtk_widget_get_pointer( widget, &x, &y );
		if ( x >= 0 && x < widget->allocation.width &&
			 y >= 0 && y < widget->allocation.height )
		{
			HandleError( g_error_copy( current_error ), Text(ERROR_GET_ALBUM_COVER) );
			return TRUE;
		}
	}

	if ( IsRadioTimeTrack || IsLastfmTrack )
	{
		widget = GTK_WIDGET(current_radio);
		gtk_widget_get_pointer( widget, &x, &y );
		if ( x >= 0 && x < widget->allocation.width &&
			 y >= 0 && y < widget->allocation.height )
		{
			show_radio_popup_menu();
			return TRUE;
		}
	}

/*TODO: Popup-Menu bei rechts-klick!!!*/

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

gint GetNowPlayingWidth( gboolean fullscreen, int album_art_size )
{
	int border_width = current_border_width;
	if ( Settings->startup.classic_ui ) border_width *= (fullscreen + 1);

	if ( album_art_size == 0 ) album_art_size = NOW_PLAYING_NOTEBOOK_IMAGE_SIZE;

	return 2 * border_width
		+ album_art_size + ALBUM_ART_SIZE_TOLERANCE
		+ CURRENT_COL_SPACINGS + ASSUMED_LABEL_SIZE;
}

void SetNowPlayingAttributes( int fullscreen, int album_art )
{
	static int current_fullscreen = -1, current_album_art = -1;

	if ( Settings->startup.classic_ui )
	{
	#ifdef MAEMO
		gboolean set_label_width = FALSE;
	#endif

		if ( fullscreen < 0 && current_fullscreen < 0 ) fullscreen = GetFullscreen();
		if ( album_art  < 0 && current_album_art  < 0 ) album_art  = TRUE;

		TRACE(( "FixNowPlaying( fullscreen = %d, album_art = %d ): current_fullscreen = %d, current_album_art = %d\n",
			fullscreen, album_art, current_fullscreen, current_album_art ));

		if ( fullscreen >= 0 && fullscreen != current_fullscreen )
		{
			gtk_container_set_border_width( GTK_CONTAINER(current_hbox), current_border_width * (fullscreen + 1) );
			current_fullscreen = fullscreen;

		#ifdef MAEMO
			set_label_width = TRUE;
		#endif
		}

	#ifdef MAEMO
		/* Workaround for Bug in GTK+ on Maemo4 */

		if ( album_art >= 0 && album_art != current_album_art )
		{
			set_label_width = TRUE;
			current_album_art = album_art;
		}

		if ( fullscreen < 0 && album_art < 0 ) set_label_width = TRUE;

		if ( set_label_width )
		{
			int width_request = ( current_fullscreen ) ? 800 : 720;  /* Nokia IT window size */

			width_request -= 2 * (current_border_width * (current_fullscreen + 1));

			if ( current_album_art )
			{
				int album_art_size = Settings->now_playing.album_art_size;
				if ( album_art_size == 0 ) album_art_size = NOW_PLAYING_NOTEBOOK_IMAGE_SIZE;

				width_request -= album_art_size + ALBUM_ART_SIZE_TOLERANCE + CURRENT_COL_SPACINGS;
			}

			TRACE(( "SetNowPlayingAttributes(): width_request = %d\n", width_request ));
			gtk_widget_set_size_request( GTK_WIDGET(current_label), width_request, -1 );
		}
	#endif
	}
}

/*-----------------------------------------------------------------------------------------------*/
