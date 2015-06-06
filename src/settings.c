/*
	settings.c
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

#ifdef WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN  /* Exclude rarely-used stuff from Windows headers */
#include <windows.h>         /* for GetModuleFileName() */
#endif

/*-----------------------------------------------------------------------------------------------*/

struct settings *Settings;
static gchar *share_dirname, *config_dirname;

void load_settings( void );
gpointer save_settings( gpointer data );

/*-----------------------------------------------------------------------------------------------*/

struct settings_widgets
{
	struct startup_widgets
	{
		GtkWidget *classic_ui, *toolbar_at_bottom;
		GtkWidget *fullscreen, *maximize;
		GtkWidget *clock_format;
	} startup;

	struct browser_widgets
	{
		GtkWidget *activate_items_with_single_click;
		GtkWidget *go_back_item;
		GtkWidget *playlist_container_action, *music_album_action, *music_track_action, *audio_broadcast_action;
		GtkWidget *page_after_replacing_playlist;

		GtkWidget *handle_storage_folder_with_artist_as_album;
		GtkWidget *handle_storage_folder_with_genre_as_album;

		GtkWidget *local_folder_path;
	} browser;

	struct playlist_widgets
	{
		GtkWidget *toolbar;
	} playlist;

	struct now_playing_widgets
	{
		GtkWidget *album_art_size, *shrink_album_art_only;
		GtkWidget *show_track_number, *show_track_duration, *show_album_year;
		GtkWidget *double_click;
	} now_playing;

	struct volume_widgets
	{
		GtkWidget *ui;
		GtkWidget *show_level;
		GtkWidget *quiet, *limit;
	} volume;

	struct keys_widgets
	{
	#ifdef MAEMO
		GtkWidget *zoom, *full_screen, *left_right, *up_down, *enter;
	#else
		GtkWidget *zoom, *full_screen;
	#endif
		GtkWidget *use_system_keys;
	} keys;

#if RADIOTIME || RADIOTIME_LINN_DS
	struct radiotime_widgets
	{
		GtkWidget *active0, *active1, *active2;
		GtkWidget *l_username, *username;
	} radiotime;
#endif

#if LASTFM
	struct lastfm_widgets
	{
		GtkWidget *active;
		GtkWidget *l_username, *username, *l_password, *password;
		GtkWidget *album_art;
		/* TODO: *last_fm_now_playing, *last_fm_scrobbling; */
		GtkWidget *discovery_mode;
	} lastfm;
#endif

	struct ssdp_widgets
	{
		GtkWidget *media_server, *media_renderer, *linn_products;
		GtkWidget *mx, *timeout;
		GtkWidget *default_media_renderer;
	} ssdp;
};

/*-----------------------------------------------------------------------------------------------*/

static GtkWidget *new_text_combo_box( int text_index0, unsigned bitmask )
{
	GtkWidget *combo_box = gtk_combo_box_new_text();

	g_object_set_data( G_OBJECT(combo_box), "bitmask", GUINT_TO_POINTER(bitmask) );

	for ( ; bitmask != 0; bitmask >>= 1 )
	{
		if ( (bitmask & 1) != 0 )
			gtk_combo_box_append_text( GTK_COMBO_BOX(combo_box), Texts[text_index0] );

		text_index0++;
	}

	return combo_box;
}

static void set_combo_box_active( GtkComboBox *combo_box, int index )
{
	unsigned bitmask = GPOINTER_TO_UINT( g_object_get_data( G_OBJECT(combo_box), "bitmask" ) );
	int i = 0;

	ASSERT( (bitmask & 1 << index) != 0 );

	while ( --index >= 0 )
	{
		if ( (bitmask & 1 << index) != 0 ) i++;
	}

	gtk_combo_box_set_active( combo_box, i );
}

static int get_combo_box_active( GtkComboBox *combo_box )
{
	unsigned bitmask = GPOINTER_TO_UINT( g_object_get_data( G_OBJECT(combo_box), "bitmask" ) );
	int i = gtk_combo_box_get_active( combo_box );
	int index = 0;

	for (;;)
	{
		if ( (bitmask & 1 << index) != 0 )
		{
			if ( --i < 0 ) break;
		}

		index++;
	}

	ASSERT( (bitmask & 1 << index) != 0 );
	return index;
}

/*-----------------------------------------------------------------------------------------------*/

static GtkWidget *create_startup_customisation( struct startup_widgets *widgets, struct startup_settings *settings )
{
	gint top_attach = 0;
	GtkTable *table;

#ifdef MAEMO

	table = new_table( 1, 2 );

#else

	if ( Settings->MID )
	{
		table = new_table( 3, 2 );

		widgets->toolbar_at_bottom = gtk_check_button_new_with_label( Text(SETTINGS_TOOLBAR_AT_BOTTOM) );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->toolbar_at_bottom), settings->toolbar_at_bottom );
		attach_to_table( table, widgets->toolbar_at_bottom, top_attach++, 0, 2 );
	}
	else
	{
		table = new_table( 4, 2 );

		widgets->classic_ui = gtk_check_button_new_with_label( Text(SETTINGS_CLASSIC_UI) );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->classic_ui), settings->classic_ui );
		attach_to_table( table, widgets->classic_ui, top_attach++, 0, 2 );
	}

#endif

	widgets->fullscreen = gtk_check_button_new_with_label( Text(SETTINGS_FULLSCREEN_MODE) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->fullscreen), settings->fullscreen );
	attach_to_table( table, widgets->fullscreen, top_attach++, 0, 2 );

#ifndef MAEMO

	if ( !Settings->MID )
	{
		widgets->maximize = gtk_check_button_new_with_label( Text(SETTINGS_MAXIMIZE_WINDOW) );
		gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->maximize), settings->maximize );
		attach_to_table( table, widgets->maximize, top_attach++, 0, 2 );
	}

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_CLOCK) ), top_attach, 0, 1 );
	widgets->clock_format = new_text_combo_box( TextIndex(SETTINGS_CLOCK_NONE), 15 );
	set_combo_box_active( GTK_COMBO_BOX(widgets->clock_format), settings->clock_format );
	attach_to_table( table, widgets->clock_format, top_attach++, 1, 2 );

#endif

	return GTK_WIDGET(table);
}

static void get_startup_customisation( struct startup_widgets *widgets, struct startup_settings *settings )
{
	if ( widgets->classic_ui != NULL ) settings->classic_ui = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->classic_ui) );
	if ( widgets->toolbar_at_bottom != NULL ) settings->toolbar_at_bottom = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->toolbar_at_bottom) );

	if ( widgets->fullscreen != NULL ) settings->fullscreen = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->fullscreen) );
	if ( widgets->maximize != NULL ) settings->maximize = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->maximize) );

	if ( widgets->clock_format != NULL ) settings->clock_format = get_combo_box_active( GTK_COMBO_BOX(widgets->clock_format) );
}

static void set_startup_defaults( struct startup_settings *settings )
{
	settings->classic_ui = Settings->MID;
	settings->toolbar_at_bottom = Settings->MID;

	settings->fullscreen = FALSE;
	settings->maximize = Settings->MID;

	settings->clock_format = 1;  /* Time representation for current locale */
}

static void load_startup( struct startup_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "ClassicUI" ) == 0 )
		settings->classic_ui = atoi( value );
	else if ( strcmp( key, "ToolbarAtBottom" ) == 0 )
		settings->toolbar_at_bottom = atoi( value );
	if ( strcmp( key, "Fullscreen" ) == 0 )
		settings->fullscreen = atoi( value );
	else if ( strcmp( key, "Maximize" ) == 0 )
		settings->maximize = atoi( value );
	else if ( strcmp( key, "ClockFormat" ) == 0 )
		settings->clock_format = atoi( value );
}

static void save_startup( FILE *fp, struct startup_settings *settings )
{
	file_put_int( fp, "ClassicUI", settings->classic_ui );
	file_put_int( fp, "ToolbarAtBottom", settings->toolbar_at_bottom );
	file_put_int( fp, "Fullscreen", settings->fullscreen );
	file_put_int( fp, "Maximize", settings->maximize );
	file_put_int( fp, "ClockFormat", settings->clock_format );
}

/*-----------------------------------------------------------------------------------------------*/

static void set_browser_defaults( struct browser_settings *settings )
{
	if ( Settings->MID )
	{
		settings->activate_items_with_single_click = TRUE;
		settings->playlist_container_action = BROWSER_POPUP_MENU;
		settings->music_album_action = BROWSER_POPUP_MENU;
		settings->music_track_action = BROWSER_POPUP_MENU;
	}
	else
	{
		settings->activate_items_with_single_click = FALSE;
		settings->playlist_container_action = BROWSER_OPEN;
		settings->music_album_action = BROWSER_OPEN;
		settings->music_track_action = BROWSER_APPEND_TO_PLAYLIST;
	}

	settings->go_back_item = TRUE;
	settings->audio_broadcast_action = BROWSER_SET_CHANNEL;
	settings->page_after_replacing_playlist = NOW_PLAYING_PAGE;

	settings->handle_storage_folder_with_artist_as_album = TRUE;
	settings->handle_storage_folder_with_genre_as_album = TRUE;

#ifndef MAEMO
	settings->local_folder = TRUE;  /*!Settings->MID;*/
#endif
#if GLIB_CHECK_VERSION(2,14,0)
	settings->local_folder_path = g_strdup( g_get_user_special_dir( G_USER_DIRECTORY_MUSIC ) );
	if ( settings->local_folder_path == NULL )
#endif
	settings->local_folder_path = g_strdup( g_get_user_data_dir() );
	TRACE(( "set_browser_defaults(): local_folder_path = \"%s\"\n", settings->local_folder_path ));
}

static void load_browser( struct browser_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "ActivateItemsWithSingleClick" ) == 0 )
		settings->activate_items_with_single_click = atoi( value );
	else if ( strcmp( key, "GoBackItem" ) == 0 )
		settings->go_back_item = atoi( value );
	else if ( strcmp( key, "PlaylistContainerAction" ) == 0 )
		settings->playlist_container_action = atoi( value );
	else if ( strcmp( key, "MusicAlbumAction" ) == 0 )
		settings->music_album_action = atoi( value );
	else if ( strcmp( key, "MusicTrackAction" ) == 0 )
		settings->music_track_action = atoi( value );
	else if ( strcmp( key, "AudioBroadcastAction" ) == 0 )
		settings->audio_broadcast_action = atoi( value );
	else if ( strcmp( key, "PageAfterReplacingPlaylist" ) == 0 )
		settings->page_after_replacing_playlist = atoi( value );

	else if ( strcmp( key, "HandleStorageFolderWithArtistAsAlbum" ) == 0 )
		settings->handle_storage_folder_with_artist_as_album = atoi( value );
	else if ( strcmp( key, "HandleStorageFolderWithGenreAsAlbum" ) == 0 )
		settings->handle_storage_folder_with_genre_as_album = atoi( value );

	else if ( strcmp( key, "LocalFolderPath" ) == 0 )
		{ g_free( settings->local_folder_path ); settings->local_folder_path = g_strdup( value ); }
}

static void save_browser( FILE *fp, struct browser_settings *settings )
{
	file_put_int( fp, "ActivateItemsWithSingleClick", settings->activate_items_with_single_click );
	file_put_int( fp, "GoBackItem", settings->go_back_item );
	file_put_int( fp, "PlaylistContainerAction", settings->playlist_container_action );
	file_put_int( fp, "MusicAlbumAction", settings->music_album_action );
	file_put_int( fp, "MusicTrackAction", settings->music_track_action );
	file_put_int( fp, "AudioBroadcastAction", settings->audio_broadcast_action );
	file_put_int( fp, "PageAfterReplacingPlaylist", settings->page_after_replacing_playlist );

	file_put_int( fp, "HandleStorageFolderWithArtistAsAlbum", settings->handle_storage_folder_with_artist_as_album );
	file_put_int( fp, "HandleStorageFolderWithGenreAsAlbum", settings->handle_storage_folder_with_genre_as_album );

	file_put_string( fp, "LocalFolderPath", settings->local_folder_path );
}

static GtkWidget *create_browser_customisation( struct browser_widgets *widgets, struct browser_settings *settings )
{
	GtkTable *table = new_table( 8, 3 );

	widgets->activate_items_with_single_click = gtk_check_button_new_with_label( Text(SETTINGS_ACTIVATE_ITEMS_WITH_SINGLE_CLICK) );
	attach_to_table( table, widgets->activate_items_with_single_click, 0, 0, 3 );

	widgets->go_back_item = gtk_check_button_new_with_label( Text(SETTINGS_GO_BACK_ITEM) );
	attach_to_table( table, widgets->go_back_item, 1, 0, 3 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_PLAYLIST_CONTAINER_ACTION) ), 2, 0, 1 );
	widgets->playlist_container_action = new_text_combo_box( TextIndex(BROWSER_POPUP_MENU),
		1 << BROWSER_POPUP_MENU | 1 << BROWSER_OPEN | 1 << BROWSER_REPLACE_PLAYLIST | 1 << BROWSER_APPEND_TO_PLAYLIST );
	attach_to_table( table, widgets->playlist_container_action, 2, 1, 3 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_MUSIC_ALBUM_ACTION) ), 3, 0, 1 );
	widgets->music_album_action = new_text_combo_box( TextIndex(BROWSER_POPUP_MENU),
		1 << BROWSER_POPUP_MENU | 1 << BROWSER_OPEN | 1 << BROWSER_REPLACE_PLAYLIST | 1 << BROWSER_APPEND_TO_PLAYLIST );
	attach_to_table( table, widgets->music_album_action, 3, 1, 3 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_MUSIC_TRACK_ACTION) ), 4, 0, 1 );
	widgets->music_track_action = new_text_combo_box( TextIndex(BROWSER_POPUP_MENU),
		1 << BROWSER_POPUP_MENU | 1 << BROWSER_REPLACE_PLAYLIST | 1 << BROWSER_APPEND_TO_PLAYLIST );
	attach_to_table( table, widgets->music_track_action, 4, 1, 3 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_AUDIO_BROADCAST_ACTION) ), 5, 0, 1 );
	widgets->audio_broadcast_action = new_text_combo_box( TextIndex(BROWSER_POPUP_MENU),
		1 << BROWSER_POPUP_MENU | 1 << BROWSER_REPLACE_PLAYLIST | 1 << BROWSER_APPEND_TO_PLAYLIST |
		1 << BROWSER_SET_CHANNEL );
	attach_to_table( table, widgets->audio_broadcast_action, 5, 1, 3 );

	attach_to_table( table, gtk_hbox_new( FALSE, 0 ), 6, 0, 3 );

	if ( Settings->startup.classic_ui )
	{
		attach_to_table( table, new_label_with_colon( Text(SETTINGS_PAGE_AFTER_REPLACING_PLAYLIST) ), 7, 0, 2 );
		widgets->page_after_replacing_playlist = new_text_combo_box( TextIndex(BROWSER),
			1 << BROWSER_PAGE | 1 << PLAYLIST_PAGE | 1 << NOW_PLAYING_PAGE );
		set_combo_box_active( GTK_COMBO_BOX(widgets->page_after_replacing_playlist), settings->page_after_replacing_playlist );
		attach_to_table( table, widgets->page_after_replacing_playlist, 7, 2, 3 );
	}

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->activate_items_with_single_click), settings->activate_items_with_single_click );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->go_back_item), settings->go_back_item );
	set_combo_box_active( GTK_COMBO_BOX(widgets->playlist_container_action), settings->playlist_container_action );
	set_combo_box_active( GTK_COMBO_BOX(widgets->music_album_action), settings->music_album_action );
	set_combo_box_active( GTK_COMBO_BOX(widgets->music_track_action), settings->music_track_action );
	set_combo_box_active( GTK_COMBO_BOX(widgets->audio_broadcast_action), settings->audio_broadcast_action );

	return GTK_WIDGET(table);
}

static void get_browser_customisation( struct browser_widgets *widgets, struct browser_settings *settings )
{
	settings->activate_items_with_single_click = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->activate_items_with_single_click) );
	settings->go_back_item = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->go_back_item) );
	settings->playlist_container_action = get_combo_box_active( GTK_COMBO_BOX(widgets->playlist_container_action) );
	settings->music_album_action = get_combo_box_active( GTK_COMBO_BOX(widgets->music_album_action) );
	settings->music_track_action = get_combo_box_active( GTK_COMBO_BOX(widgets->music_track_action) );
	settings->audio_broadcast_action = get_combo_box_active( GTK_COMBO_BOX(widgets->audio_broadcast_action) );

	if ( widgets->page_after_replacing_playlist != NULL )
		settings->page_after_replacing_playlist = get_combo_box_active( GTK_COMBO_BOX(widgets->page_after_replacing_playlist) );
}

static GtkWidget *create_browser_options( struct browser_widgets *widgets, struct browser_settings *settings )
{
	GtkTable *table = new_table( 3, 2 );

	widgets->handle_storage_folder_with_artist_as_album = gtk_check_button_new_with_label( Text(SETTINGS_HANDLE_STORAGE_FOLDER_WITH_ARTIST_AS_ALBUM) );
	attach_to_table( table, widgets->handle_storage_folder_with_artist_as_album, 0, 0, 2 );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->handle_storage_folder_with_artist_as_album), settings->handle_storage_folder_with_artist_as_album );

	widgets->handle_storage_folder_with_genre_as_album = gtk_check_button_new_with_label( Text(SETTINGS_HANDLE_STORAGE_FOLDER_WITH_GENRE_AS_ALBUM) );
	attach_to_table( table, widgets->handle_storage_folder_with_genre_as_album, 1, 0, 2 );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->handle_storage_folder_with_genre_as_album), settings->handle_storage_folder_with_genre_as_album );

	if ( settings->local_folder )
	{
		attach_to_table( table, new_label_with_colon( Text(BROWSER_LOCAL_FOLDER) ), 2, 0, 1 );
		widgets->local_folder_path = gtk_file_chooser_button_new( "Select picture folder", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );
		attach_to_table( table, widgets->local_folder_path, 2, 1, 2 );
		gtk_file_chooser_set_filename( GTK_FILE_CHOOSER(widgets->local_folder_path), settings->local_folder_path );
	}

	return GTK_WIDGET(table);
}

static void get_browser_options( struct browser_widgets *widgets, struct browser_settings *settings )
{
	settings->handle_storage_folder_with_artist_as_album = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->handle_storage_folder_with_artist_as_album) );
	settings->handle_storage_folder_with_genre_as_album = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->handle_storage_folder_with_genre_as_album) );

	if ( widgets->local_folder_path != NULL )
	{
		g_free( settings->local_folder_path );
		settings->local_folder_path = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(widgets->local_folder_path) );
	}
}

/*-----------------------------------------------------------------------------------------------*/

static void set_playlist_defaults( struct playlist_settings *settings )
{
	settings->toolbar = PLAYLIST_RIGHT_TOOLBAR;
}


static void load_playlist( struct playlist_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "CurrentFolder" ) == 0 )
		settings->current_folder = g_strdup( value );
	else if ( strcmp( key, "Toolbar" ) == 0 )
		settings->toolbar = atoi( value );
}

static void save_playlist( FILE *fp, struct playlist_settings *settings )
{
	file_put_string( fp, "CurrentFolder", settings->current_folder );
	file_put_int( fp, "Toolbar", settings->toolbar );
}

static GtkWidget *create_playlist_customisation( struct playlist_widgets *widgets, struct playlist_settings *settings )
{
	GtkTable *table = new_table( 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_PLAYLIST_TOOLBAR) ), 0, 0, 1 );
	widgets->toolbar = new_text_combo_box( TextIndex(PLAYLIST_NO_TOOLBAR),
		1 << PLAYLIST_NO_TOOLBAR | 1 << PLAYLIST_LEFT_TOOLBAR | 1 << PLAYLIST_RIGHT_TOOLBAR );
	set_combo_box_active( GTK_COMBO_BOX(widgets->toolbar), settings->toolbar );
	attach_to_table( table, widgets->toolbar, 0, 1, 2 );

	return GTK_WIDGET(table);
}

static void get_playlist_customisation( struct playlist_widgets *widgets, struct playlist_settings *settings )
{
	settings->toolbar = get_combo_box_active( GTK_COMBO_BOX(widgets->toolbar) );
}

/*-----------------------------------------------------------------------------------------------*/

static void set_now_playing_defaults( struct now_playing_settings *settings )
{
	settings->album_art_size = ( Settings->startup.classic_ui )
		? NOW_PLAYING_NOTEBOOK_IMAGE_SIZE
		: NOW_PLAYING_PANED_IMAGE_SIZE;
	settings->shrink_album_art_only = FALSE;
	settings->show_track_number = TRUE;
	settings->show_track_duration = TRUE;
	settings->show_album_year = TRUE;
#ifdef MAEMO
	settings->double_click = KEYBD_NOP;
#else
	settings->double_click = KEYBD_SELECTION;
#endif
}

static void load_now_playing( struct now_playing_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "AlbumArtSize" ) == 0 )
		settings->album_art_size = atoi( value );
	else if ( strcmp( key, "ShrinkAlbumArtOnly" ) == 0 )
		settings->shrink_album_art_only = atoi( value );
	else if ( strcmp( key, "ShowTrackNumber" ) == 0 )
		settings->show_track_number = atoi( value );
	else if ( strcmp( key, "ShowTrackDuration" ) == 0 )
		settings->show_track_duration = atoi( value );
	else if ( strcmp( key, "ShowAlbumYear" ) == 0 )
		settings->show_album_year = atoi( value );
#ifndef MAEMO
	else if ( strcmp( key, "DoubleClick" ) == 0 )
		settings->double_click = atoi( value );
#endif
}

static void save_now_playing( FILE *fp, struct now_playing_settings *settings )
{
	file_put_int( fp, "AlbumArtSize", settings->album_art_size );
	file_put_int( fp, "ShrinkAlbumArtOnly", settings->shrink_album_art_only );
	file_put_int( fp, "ShowTrackNumber", settings->show_track_number );
	file_put_int( fp, "ShowTrackDuration", settings->show_track_duration );
	file_put_int( fp, "ShowAlbumYear", settings->show_album_year );
	file_put_int( fp, "DoubleClick", settings->double_click );
}

static GtkWidget *create_now_playing_customisation( struct now_playing_widgets *widgets, struct now_playing_settings *settings )
{
	GtkTable *table = new_table( 7, 2 );
	int image_size, index;

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_ALBUM_ART_SIZE) ), 0, 0, 1 );
	widgets->album_art_size = gtk_combo_box_new_text();
	gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->album_art_size), "0" );
	for ( image_size = NOW_PLAYING_MINIMUM_IMAGE_SIZE;; image_size += 100 )
	{
		gint width  = GetNowPlayingWidth( TRUE, image_size );
		gint height = image_size + 180 /* Space for Toolbar & Statusbar */;
		char buf[8];

		if ( width > Settings->screen_width || height > Settings->screen_height )
			break;

		sprintf( buf, "%d", image_size );
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->album_art_size), buf );
	}
	attach_to_table( table, widgets->album_art_size, 0, 1, 2 );

	widgets->shrink_album_art_only = gtk_check_button_new_with_label( Text(SETTINGS_SHRINK_ALBUM_ART_ONLY) );
	attach_to_table( table, widgets->shrink_album_art_only, 1, 0, 2 );

	widgets->show_track_number = gtk_check_button_new_with_label( Text(SETTINGS_SHOW_TRACK_NUMBER) );
	attach_to_table( table, widgets->show_track_number, 2, 0, 2 );

	widgets->show_track_duration = gtk_check_button_new_with_label( Text(SETTINGS_SHOW_TRACK_DURATION) );
	attach_to_table( table, widgets->show_track_duration, 3, 0, 2 );

	widgets->show_album_year = gtk_check_button_new_with_label( Text(SETTINGS_SHOW_ALBUM_YEAR) );
	attach_to_table( table, widgets->show_album_year, 4, 0, 2 );

#ifndef MAEMO
	attach_to_table( table, new_label_with_colon( Text(SETTINGS_DOUBLE_CLICK) ), 6, 0, 1 );
	widgets->double_click = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_FULLSCREEN | 1 << KEYBD_PLAY_PAUSE | 1 << KEYBD_MUTE |
		1 << KEYBD_SELECTION | 1 << KEYBD_SELECT_RENDERER );
	attach_to_table( table, widgets->double_click, 6, 1, 2 );
#endif

	index = ( settings->album_art_size == 0 ) ? 0 : 1 + (settings->album_art_size - NOW_PLAYING_MINIMUM_IMAGE_SIZE) / 100;
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->album_art_size), index );

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->shrink_album_art_only), settings->shrink_album_art_only );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->show_track_number), settings->show_track_number );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->show_track_duration), settings->show_track_duration );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->show_album_year), settings->show_album_year );

#ifndef MAEMO
	set_combo_box_active( GTK_COMBO_BOX(widgets->double_click), settings->double_click );
#endif

	return GTK_WIDGET(table);
}

static void get_now_playing_customisation( struct now_playing_widgets *widgets, struct now_playing_settings *settings )
{
	int index = gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->album_art_size) );
	if ( index == 0 )
		settings->album_art_size = 0;
	else if ( index > 0 )
		settings->album_art_size = NOW_PLAYING_MINIMUM_IMAGE_SIZE + (index - 1) * 100;

	settings->shrink_album_art_only = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->shrink_album_art_only) );
	settings->show_track_number = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->show_track_number) );
	settings->show_track_duration = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->show_track_duration) );
	settings->show_album_year = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->show_album_year) );

#ifndef MAEMO
	settings->double_click = get_combo_box_active( GTK_COMBO_BOX(widgets->double_click) );
#endif
}

/*-----------------------------------------------------------------------------------------------*/

static void set_volume_defaults( struct volume_settings *settings )
{
	settings->ui = VOLUME_UI_BAR;
	settings->show_level = FALSE;
	settings->quiet = 10;
	settings->limit = 100;
}

static void load_volume( struct volume_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "UI" ) == 0 )
		settings->ui = atoi( value );
	else if ( strcmp( key, "ShowLevel" ) == 0 )
		settings->show_level = atoi( value );
	else if ( strcmp( key, "Quiet" ) == 0 )
		settings->quiet = atoi( value );
	else if ( strcmp( key, "Limit" ) == 0 )
		settings->limit = atoi( value );
}

static void save_volume( FILE *fp, struct volume_settings *settings )
{
	file_put_int( fp, "UI", settings->ui );
	file_put_int( fp, "ShowLevel", settings->show_level );
	file_put_int( fp, "Quiet", settings->quiet );
	file_put_int( fp, "Limit", settings->limit );
}

static GtkWidget *create_volume_customisation( struct volume_widgets *widgets, struct volume_settings *settings )
{
	GtkTable *table = new_table( 5, 2 );
	char value[4];
	int i;

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_VOLUME_UI) ), 0, 0, 1 );
#ifdef GTK_TYPE_VOLUME_BUTTON
	widgets->ui = new_text_combo_box( TextIndex(SETTINGS_VOLUME_UI_NONE),
		1 << VOLUME_UI_NONE | 1 << VOLUME_UI_PLUSMINUS | 1 << VOLUME_UI_BUTTON | 1 << VOLUME_UI_BAR );
#else
	widgets->ui = new_text_combo_box( TextIndex(SETTINGS_VOLUME_UI_NONE),
		1 << VOLUME_UI_NONE | 1 << VOLUME_UI_PLUSMINUS | 1 << VOLUME_UI_BAR );
#endif
	attach_to_table( table, widgets->ui, 0, 1, 2 );

	widgets->show_level = gtk_check_button_new_with_label( Text(SETTINGS_VOLUME_SHOW_LEVEL) );
	attach_to_table( table, widgets->show_level, 1, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_VOLUME_QUIET) ), 3, 0, 1 );
	widgets->quiet = gtk_combo_box_new_text();
	for ( i = 0; i <= 10; i++ )
	{
		sprintf( value, "%d", i );
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->quiet), value );
	}
	attach_to_table( table, widgets->quiet, 3, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_VOLUME_LIMIT) ), 4, 0, 1 );
	widgets->limit = gtk_combo_box_new_text();
	for ( i = 10; i <= 100; i += 10 )
	{
		sprintf( value, "%d", i );
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->limit), value );
	}
	attach_to_table( table, widgets->limit, 4, 1, 2 );

	set_combo_box_active( GTK_COMBO_BOX(widgets->ui), settings->ui );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->show_level), settings->show_level );
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->quiet), settings->quiet );
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->limit), (settings->limit / 10) - 1 );

	return GTK_WIDGET(table);
}

static void get_volume_customisation( struct volume_widgets *widgets, struct volume_settings *settings )
{
	settings->ui = get_combo_box_active( GTK_COMBO_BOX(widgets->ui) );
	settings->show_level = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->show_level) );
	settings->quiet = gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->quiet) );
	settings->limit = (gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->limit) ) + 1) * 10;
}

/*-----------------------------------------------------------------------------------------------*/

static void set_keys_defaults( struct keys_settings *settings )
{
	settings->zoom = KEYBD_VOLUME;
	settings->full_screen = KEYBD_SELECTION;
	settings->left_right = KEYBD_PAGE;
	settings->up_down = KEYBD_TRACK;
	settings->enter = KEYBD_PLAY_PAUSE;
	settings->use_system_keys = !IsSmartQ();
}

static void load_keys( struct keys_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "Zoom" ) == 0 )
		settings->zoom = atoi( value );
	else if ( strcmp( key, "FullScreen" ) == 0 )
		settings->full_screen = atoi( value );
	else if ( strcmp( key, "LeftRight" ) == 0 )
		settings->left_right = atoi( value );
	else if ( strcmp( key, "UpDown" ) == 0 )
		settings->up_down = atoi( value );
	else if ( strcmp( key, "Enter" ) == 0 )
		settings->enter = atoi( value );
	else if ( strcmp( key, "UseSystemKeys" ) == 0 )
		settings->use_system_keys = atoi( value );
}

static void save_keys( FILE *fp, struct keys_settings *settings )
{
	file_put_int( fp, "Zoom", settings->zoom );
	file_put_int( fp, "FullScreen", settings->full_screen );
	file_put_int( fp, "LeftRight", settings->left_right );
	file_put_int( fp, "UpDown", settings->up_down );
	file_put_int( fp, "Enter", settings->enter );
	file_put_int( fp, "UseSystemKeys", settings->use_system_keys );
}

static GtkWidget *create_keys_customisation( struct keys_widgets *widgets, struct keys_settings *settings )
{
#ifdef MAEMO

	GtkTable *table = new_table( 6, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_ZOOM_KEY) ), 0, 0, 1 );
	widgets->zoom = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_PAGE | 1 << KEYBD_TRACK | 1 << KEYBD_VOLUME );
	attach_to_table( table, widgets->zoom, 0, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_FULL_SCREEN_KEY) ), 1, 0, 1 );
	widgets->full_screen = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_FULLSCREEN | 1 << KEYBD_PLAY_PAUSE | 1 << KEYBD_MUTE |
		1 << KEYBD_SELECTION | 1 << KEYBD_SELECT_RENDERER );
	attach_to_table( table, widgets->full_screen, 1, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_LEFT_RIGHT_KEY) ), 2, 0, 1 );
	widgets->left_right = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_PAGE | 1 << KEYBD_TRACK | 1 << KEYBD_VOLUME );
	attach_to_table( table, widgets->left_right, 2, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_UP_DOWN_KEY) ), 3, 0, 1 );
	widgets->up_down = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_PAGE | 1 << KEYBD_TRACK | 1 << KEYBD_VOLUME );
	attach_to_table( table, widgets->up_down, 3, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_ENTER_KEY) ), 4, 0, 1 );
	widgets->enter = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_PLAY_PAUSE | 1 << KEYBD_MUTE );
	attach_to_table( table, widgets->enter, 4, 1, 2 );

	widgets->use_system_keys = gtk_check_button_new_with_label( Text(SETTINGS_USE_SYSTEM_KEYS) );
	attach_to_table( table, widgets->use_system_keys, 5, 0, 2 );

#else

	GtkTable *table = new_table( 3, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_PAGE_UP_DOWN_KEY) ), 0, 0, 1 );
	widgets->zoom = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_PAGE | 1 << KEYBD_TRACK | 1 << KEYBD_VOLUME );
	attach_to_table( table, widgets->zoom, 0, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_FULL_SCREEN_KEY) ), 1, 0, 1 );
	widgets->full_screen = new_text_combo_box( TextIndex(KEYBD_NOP),
		1 << KEYBD_NOP | 1 << KEYBD_FULLSCREEN | 1 << KEYBD_PLAY_PAUSE | 1 << KEYBD_MUTE |
		1 << KEYBD_SELECTION | 1 << KEYBD_SELECT_RENDERER );
	attach_to_table( table, widgets->full_screen, 1, 1, 2 );

	widgets->use_system_keys = gtk_check_button_new_with_label( Text(SETTINGS_USE_SYSTEM_KEYS) );
	attach_to_table( table, widgets->use_system_keys, 2, 0, 2 );

#endif

	set_combo_box_active( GTK_COMBO_BOX(widgets->zoom), settings->zoom );
	set_combo_box_active( GTK_COMBO_BOX(widgets->full_screen), settings->full_screen );
#ifdef MAEMO
	set_combo_box_active( GTK_COMBO_BOX(widgets->up_down), settings->up_down );
	set_combo_box_active( GTK_COMBO_BOX(widgets->left_right), settings->left_right );
	set_combo_box_active( GTK_COMBO_BOX(widgets->enter), settings->enter );
#endif
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->use_system_keys), settings->use_system_keys );

	return GTK_WIDGET(table);
}

static void get_keys_customisation( struct keys_widgets *widgets, struct keys_settings *settings )
{
	if ( widgets->zoom != NULL )
	{
		settings->zoom = get_combo_box_active( GTK_COMBO_BOX(widgets->zoom) );
		settings->full_screen = get_combo_box_active( GTK_COMBO_BOX(widgets->full_screen) );
	#ifdef MAEMO
		settings->up_down = get_combo_box_active( GTK_COMBO_BOX(widgets->up_down) );
		settings->left_right = get_combo_box_active( GTK_COMBO_BOX(widgets->left_right) );
		settings->enter = get_combo_box_active( GTK_COMBO_BOX(widgets->enter) );
	#endif
		settings->use_system_keys = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->use_system_keys) );
	}
}

/*-----------------------------------------------------------------------------------------------*/

#if RADIOTIME || RADIOTIME_LINN_DS

static void set_radiotime_defaults( struct radiotime_settings *settings )
{
#if RADIOTIME_LINN_DS
	settings->active = 2;
#else
	;   /* nothing to do */
#endif
}

static void load_radiotime( struct radiotime_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "Active" ) == 0 )
		settings->active = atoi( value );
	else if ( strcmp( key, "Username" ) == 0 )
		strcpy( settings->username, value );

	if ( settings->active == 1 && strcmp( settings->username, "Linn DS" ) == 0 )
	{
		settings->active = 2;
		settings->username[0] = '\0';
	}
}

static void save_radiotime( FILE *fp, struct radiotime_settings *settings )
{
	file_put_int( fp, "Active", settings->active );
	file_put_string( fp, "Username", settings->username );
}

#if RADIOTIME
static void radiotime_toggled( GtkToggleButton *togglebutton, const struct radiotime_widgets *widgets )
{
	gboolean active = gtk_toggle_button_get_active( togglebutton );

	if ( active )
	{
		gboolean username_sensitive = GTK_WIDGET(togglebutton) == widgets->active1;
		gtk_widget_set_sensitive( widgets->l_username, username_sensitive );
		gtk_widget_set_sensitive( widgets->username, username_sensitive );
	}
}
#endif

static GtkWidget *create_radiotime_options( struct radiotime_widgets *widgets, struct radiotime_settings *settings )
{
	GtkTable *table = new_table( 5, 2 );
	guint top_attach = 0;

	widgets->active0 = gtk_radio_button_new_with_label_from_widget( NULL, Text(SETTINGS_RADIOTIME_0) );
	attach_to_table( table, widgets->active0, top_attach++, 0, 2 );

#if RADIOTIME
	widgets->active1 = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(widgets->active0), Text(SETTINGS_RADIOTIME_1) );
	attach_to_table( table, widgets->active1, top_attach++, 0, 2 );
#endif

#if RADIOTIME_LINN_DS
	widgets->active2 = gtk_radio_button_new_with_label_from_widget( GTK_RADIO_BUTTON(widgets->active0), Text(SETTINGS_RADIOTIME_2) );
	attach_to_table( table, widgets->active2, top_attach++, 0, 2 );
#endif

#if RADIOTIME
	widgets->l_username = new_label_with_colon( Text(SETTINGS_USERNAME) );
	attach_to_table( table, widgets->l_username, ++top_attach, 0, 1 );

	widgets->username = new_entry( sizeof( Settings->radiotime.username ) - 1 );
	attach_to_table( table, widgets->username, top_attach, 1, 2 );
#endif

	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->active0), settings->active == 0 );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->active1), settings->active == 1 );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->active2), settings->active == 2 );

#if RADIOTIME
	gtk_entry_set_text( GTK_ENTRY(widgets->username), settings->username );

	radiotime_toggled( GTK_TOGGLE_BUTTON(widgets->active0), (gpointer)widgets );
	radiotime_toggled( GTK_TOGGLE_BUTTON(widgets->active1), (gpointer)widgets );
	radiotime_toggled( GTK_TOGGLE_BUTTON(widgets->active2), (gpointer)widgets );
	g_signal_connect( G_OBJECT(widgets->active0), "toggled", G_CALLBACK(radiotime_toggled), (gpointer)widgets );
	g_signal_connect( G_OBJECT(widgets->active1), "toggled", G_CALLBACK(radiotime_toggled), (gpointer)widgets );
	g_signal_connect( G_OBJECT(widgets->active2), "toggled", G_CALLBACK(radiotime_toggled), (gpointer)widgets );
#endif

	return GTK_WIDGET(table);
}

static void get_radiotime_options( struct radiotime_widgets *widgets, struct radiotime_settings *settings )
{
	if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->active0) ) )
		settings->active = 0;
	else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->active1) ) )
		settings->active = 1;
	else if ( gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->active2) ) )
		settings->active = 2;

#if RADIOTIME
	strcpy( settings->username, gtk_entry_get_text( GTK_ENTRY(widgets->username) ) );
	if ( settings->active == 1 && settings->username[0] == '\0' ) settings->active = 0;
#endif
}

#endif

/*-----------------------------------------------------------------------------------------------*/

#if LASTFM

static void set_lastfm_defaults( struct lastfm_settings *settings )
{
	settings->album_art = TRUE;
	settings->now_playing = TRUE;
	settings->scrobbling = TRUE;
	settings->discovery_mode = TRUE;
}

static void load_lastfm( struct lastfm_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "Active" ) == 0 )
		settings->active = atoi( value );
	else if ( strcmp( key, "Username" ) == 0 )
		strcpy( settings->username, value );
	else if ( strcmp( key, "Password" ) == 0 )
		strcpy( settings->password, value );
	else if ( strcmp( key, "AlbumArt" ) == 0 )
		settings->album_art = atoi( value );
	else if ( strcmp( key, "NowPlaying" ) == 0 )
		settings->now_playing = atoi( value );
	else if ( strcmp( key, "Scrobbling" ) == 0 )
		settings->scrobbling = atoi( value );
	else if ( strcmp( key, "DiscoveryMode" ) == 0 )
		settings->discovery_mode = atoi( value );
}

static void save_lastfm( FILE *fp, struct lastfm_settings *settings )
{
	file_put_int( fp, "Active", settings->active );
	file_put_string( fp, "Username", settings->username );
	file_put_string( fp, "Password", settings->password );
	file_put_int( fp, "AlbumArt", settings->album_art );
	file_put_int( fp, "NowPlaying", settings->now_playing );
	file_put_int( fp, "Scrobbling", settings->scrobbling );
	file_put_int( fp, "DiscoveryMode", settings->discovery_mode );
}

static void lastfm_toggled( GtkToggleButton *togglebutton, const struct lastfm_widgets *widgets )
{
	gboolean active = gtk_toggle_button_get_active( togglebutton );

	gtk_widget_set_sensitive( widgets->l_username, active );
	gtk_widget_set_sensitive( widgets->username, active );
	gtk_widget_set_sensitive( widgets->l_password, active );
	gtk_widget_set_sensitive( widgets->password, active );
	gtk_widget_set_sensitive( widgets->album_art, active );
#if 0
	gtk_widget_set_sensitive( widgets->now_playing, active );
	gtk_widget_set_sensitive( widgets->scrobbling, active );
#endif
	gtk_widget_set_sensitive( widgets->discovery_mode, active );
}

static GtkWidget *create_lastfm_options( struct lastfm_widgets *widgets, struct lastfm_settings *settings )
{
	GtkTable *table = new_table( 5 /*7*/, 2 );

	widgets->active = gtk_check_button_new_with_label( Text(SETTINGS_LASTFM) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->active), settings->active );
	attach_to_table( table, widgets->active, 0, 0, 2 );

	widgets->l_username = new_label_with_colon( Text(SETTINGS_USERNAME) );
	attach_to_table( table, widgets->l_username, 1, 0, 1 );

	widgets->username = new_entry( sizeof( Settings->lastfm.username ) - 1 );
	gtk_entry_set_text( GTK_ENTRY(widgets->username), settings->username );
	attach_to_table( table, widgets->username, 1, 1, 2 );

	widgets->l_password = new_label_with_colon( Text(SETTINGS_PASSWORD) );
	attach_to_table( table, widgets->l_password, 2, 0, 1 );

	widgets->password = gtk_entry_new();
	gtk_entry_set_text( GTK_ENTRY(widgets->password), ( settings->password[0] != '\0' ) ? "***" : "" );
	attach_to_table( table, widgets->password, 2, 1, 2 );

	widgets->album_art = gtk_check_button_new_with_label( Text(SETTINGS_GET_MISSING_ALBUM_ART) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->album_art), settings->album_art );
	attach_to_table( table, widgets->album_art, 3, 0, 2 );

#if 0
	widgets->now_playing = gtk_check_button_new_with_label( Text(SETTINGS_NOW_PLAYING_SUBMISSION) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->now_playing), settings->now_playing );
	attach_to_table( table, widgets->now_playing, 4, 0, 2 );

	widgets->scrobbling = gtk_check_button_new_with_label( Text(SETTINGS_SCROBBLING) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->scrobbling), settings->scrobbling );
	attach_to_table( table, widgets->scrobbling, 5, 0, 2 );
#endif

	widgets->discovery_mode = gtk_check_button_new_with_label( Text(SETTINGS_DISCOVERY_MODE) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->discovery_mode), settings->discovery_mode );
	attach_to_table( table, widgets->discovery_mode, 4 /*6*/, 0, 2 );

	lastfm_toggled( GTK_TOGGLE_BUTTON(widgets->active), (gpointer)widgets );
	g_signal_connect( G_OBJECT(widgets->active), "toggled", G_CALLBACK(lastfm_toggled), (gpointer)widgets );

	return GTK_WIDGET(table);
}

static void get_lastfm_options( struct lastfm_widgets *widgets, struct lastfm_settings *settings )
{
	const gchar *password;

	settings->active = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->active) );

	strcpy( settings->username, gtk_entry_get_text( GTK_ENTRY(widgets->username) ) );
	password = gtk_entry_get_text( GTK_ENTRY(widgets->password) );
	if ( *password != '\0' )
	{
		const gchar *s;
		for ( s = password; *s == '*'; s++ ) ;
		if ( *s != '\0' ) md5cpy( settings->password, password );
	}
	else
	{
		settings->password[0] = '\0';
	}

	settings->album_art = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->album_art) );
#if 0
	settings->now_playing = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->now_playing) );
	settings->scrobbling = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->scrobbling) );
#endif
	settings->discovery_mode = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->discovery_mode) );
}

#endif

/*-----------------------------------------------------------------------------------------------*/

static void set_ssdp_defaults( struct ssdp_settings *settings )
{
	settings->media_server = settings->media_renderer = settings->linn_products = TRUE;
	settings->mx = SSDP_MX;
	settings->timeout = (int)SSDP_TIMEOUT;
}

static void load_ssdp( struct ssdp_settings *settings, const char *key, const char *value )
{
	if ( strcmp( key, "MediaServer" ) == 0 )
		settings->media_server = atoi( value );
	else if ( strcmp( key, "MediaRenderer" ) == 0 )
		settings->media_renderer = atoi( value );
	else if ( strcmp( key, "LinnProducts" ) == 0 )
		settings->linn_products = atoi( value );
	else if ( strcmp( key, "MX" ) == 0 )
		settings->mx = atoi( value );
	else if ( strcmp( key, "Timeout" ) == 0 )
		settings->timeout = atoi( value );
	else if ( strcmp( key, "DefaultMediaRenderer" ) == 0 )
		settings->default_media_renderer = g_strdup( value );
}

static void save_ssdp( FILE *fp, struct ssdp_settings *settings )
{
	file_put_int( fp, "MediaServer", settings->media_server );
	file_put_int( fp, "MediaRenderer", settings->media_renderer );
	file_put_int( fp, "LinnProducts", settings->linn_products );
	file_put_int( fp, "MX", settings->mx );
	file_put_int( fp, "Timeout", settings->timeout );
	file_put_string( fp, "DefaultMediaRenderer", settings->default_media_renderer );
}

static void ssdp_toggled( GtkToggleButton *togglebutton, const struct ssdp_widgets *widgets )
{
	if ( !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->media_renderer) ) &&
	     !gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->linn_products) ) )
	{
		gtk_toggle_button_set_active( togglebutton, TRUE );
	}
}

static GtkWidget *create_ssdp_options( struct ssdp_widgets *widgets, struct ssdp_settings *settings )
{
	struct device_list_entry *sr_list;
	int default_media_renderer, i, n;

	GtkTable *table = new_table( 7, 2 );
	char value[4];

	widgets->media_server = gtk_check_button_new_with_label( Text(SETTINGS_SSDP_MEDIA_SERVER) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->media_server), settings->media_server );
	attach_to_table( table, widgets->media_server, 0, 0, 2 );

	widgets->media_renderer = gtk_check_button_new_with_label( Text(SETTINGS_SSDP_MEDIA_RENDERER) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->media_renderer), settings->media_renderer );
	attach_to_table( table, widgets->media_renderer, 1, 0, 2 );

	widgets->linn_products = gtk_check_button_new_with_label( Text(SETTINGS_SSDP_LINN_PRODUCTS) );
	gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(widgets->linn_products), settings->linn_products );
	attach_to_table( table, widgets->linn_products, 2, 0, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_SSDP_MX) ), 4, 0, 1 );
	widgets->mx = gtk_combo_box_new_text();
	for ( i = 3; i <= 10; i++ )
	{
		sprintf( value, "%ds", i );
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->mx), value );
	}
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->mx), settings->mx - 1 );
	attach_to_table( table, widgets->mx, 4, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_SSDP_TIMEOUT) ), 5, 0, 1 );
	widgets->timeout = gtk_combo_box_new_text();
	for ( i = 10; i <= 60; i += 10 )
	{
		sprintf( value, "%ds", i );
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->timeout), value );
	}
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->timeout), (settings->timeout / 10) - 1 );
	attach_to_table( table, widgets->timeout, 5, 1, 2 );

	attach_to_table( table, new_label_with_colon( Text(SETTINGS_DEFAULT_RENDERER) ), 6, 0, 1 );
	widgets->default_media_renderer = gtk_combo_box_new_text();
	gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->default_media_renderer), Text(SETTINGS_NO_DEFAULT_RENDERER) );

	default_media_renderer = -1;

	sr_list = GetRendererList( &n );
	for ( i = 0; i < n; i++ )
	{
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->default_media_renderer), sr_list[i].device->friendlyName );
		if ( IsDefaultMediaRenderer( sr_list[i].device ) ) default_media_renderer = i;
	}
	g_free( sr_list );

	if ( default_media_renderer < 0 && settings->default_media_renderer != NULL )
	{
		gtk_combo_box_append_text( GTK_COMBO_BOX(widgets->default_media_renderer), settings->default_media_renderer );
		default_media_renderer = n;
	}
	gtk_combo_box_set_active( GTK_COMBO_BOX(widgets->default_media_renderer), 1 + default_media_renderer );

	attach_to_table( table, widgets->default_media_renderer, 6, 1, 2 );

	g_signal_connect( G_OBJECT(widgets->media_renderer), "toggled", G_CALLBACK(ssdp_toggled), (gpointer)widgets );
	g_signal_connect( G_OBJECT(widgets->linn_products), "toggled", G_CALLBACK(ssdp_toggled), (gpointer)widgets );

	return GTK_WIDGET(table);
}

static void get_ssdp_options( struct ssdp_widgets *widgets, struct ssdp_settings *settings )
{
	struct device_list_entry *sr_list;
	int default_media_renderer, n;

	settings->media_server = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->media_server) );
	settings->media_renderer = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->media_renderer) );
	settings->linn_products = gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(widgets->linn_products) );

	settings->mx = gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->mx) ) + 1;
	settings->timeout = (gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->timeout) ) + 1) * 10;

	default_media_renderer = gtk_combo_box_get_active( GTK_COMBO_BOX(widgets->default_media_renderer) ) - 1;
	if ( default_media_renderer < 0 )
	{
		g_free( settings->default_media_renderer );
		settings->default_media_renderer = NULL;
	}
	else
	{
		sr_list = GetRendererList( &n );
		if ( default_media_renderer < n )
		{
			g_free( settings->default_media_renderer );
			settings->default_media_renderer = g_strdup( sr_list[default_media_renderer].device->UDN );
		}
		g_free( sr_list );
	}
}

/*-----------------------------------------------------------------------------------------------*/

void LoadSettings( void )
{
	gboolean classic_ui;

	TRACE(( "LoadSettings()\n" ));

	Settings = g_new0( struct settings, 1 );
	set_screen_settings();

	set_startup_defaults( &Settings->startup );
	set_browser_defaults( &Settings->browser );
	set_playlist_defaults( &Settings->playlist );
	set_now_playing_defaults( &Settings->now_playing );
	set_volume_defaults( &Settings->volume );
	set_keys_defaults( &Settings->keys );

#if RADIOTIME || RADIOTIME_LINN_DS
	set_radiotime_defaults( &Settings->radiotime );
#endif
#if LASTFM
	set_lastfm_defaults( &Settings->lastfm );
#endif

	set_ssdp_defaults( &Settings->ssdp );

	classic_ui = Settings->startup.classic_ui;
	Settings->startup.classic_ui = -1;

	load_settings();

	if ( Settings->startup.classic_ui < 0 )
	{
		Settings->startup.classic_ui = classic_ui;

		if ( !Settings->startup.classic_ui )
		{
			/* Set revised default browser values */
			Settings->browser.activate_items_with_single_click = FALSE;
			Settings->browser.playlist_container_action = BROWSER_OPEN;
			Settings->browser.music_album_action = BROWSER_OPEN;
			Settings->browser.music_track_action = BROWSER_APPEND_TO_PLAYLIST;
		}
	}
}

static gboolean settings_saved( gpointer data )
{
	gdk_threads_enter();

	show_information( NULL, NULL, Text(SETTINGS_SAVED) );

	gdk_threads_leave();
	return FALSE;
}

struct save_settings_data
{
	GSourceFunc success_function;
};

void SaveSettings( gboolean show_information )
{
	struct save_settings_data *data = g_new( struct save_settings_data, 1 );

	data->success_function = (show_information) ? settings_saved : NULL;
	g_thread_create_full( save_settings, data, 0, FALSE, FALSE, G_THREAD_PRIORITY_LOW, NULL );
}

void SetShareDir( const char *dirname )
{
	ASSERT( dirname != NULL );
	TRACE(( "SetShareDir( dirname = \"%s\" )\n", dirname ));

	if ( share_dirname != NULL ) g_free( share_dirname );
	share_dirname = g_strdup( dirname );
}

void SetUserConfigDir( const char *dirname )
{
	ASSERT( dirname != NULL );
	TRACE(( "SetUserConfigDir( dirname = \"%s\" )\n", dirname ));

	if ( config_dirname != NULL ) g_free( config_dirname );
	config_dirname = g_strdup( dirname );
}

/*-----------------------------------------------------------------------------------------------*/

#ifdef WIN32
static const char leiads_dir[] = "Leia DS";
/* g_get_user_config_dir() = "C:\Documents and Settings\User\Application Data" */
#else
static const char leiads_dir[] = "leiads";
/* g_get_user_config_dir() = "/home/user/.config" */
#endif

gchar *BuildUserConfigFilename( const char *filename )
{
	if ( config_dirname == NULL )
	{
		const char *user_config_dir = g_get_user_config_dir();
		g_mkdir( user_config_dir, 0777 );

		config_dirname = g_build_filename( user_config_dir, leiads_dir, NULL );
		ASSERT( config_dirname != NULL );
		g_mkdir( config_dirname, 0777 );
	}

	return g_build_filename( config_dirname, filename, NULL );
}

gchar *BuildApplDataFilename( const char *filename )
{
	if ( share_dirname == NULL )
	{
	#ifdef WIN32

		char module_file_name[MAX_PATH];
		gchar *utf8_module_file_name;

		GetModuleFileName( NULL, module_file_name, sizeof( module_file_name ) );
		utf8_module_file_name = g_locale_to_utf8( module_file_name, -1, NULL, NULL, NULL );
		share_dirname = g_path_get_dirname( utf8_module_file_name );
		g_free( utf8_module_file_name );

	#elif defined( MAEMO )

		share_dirname = g_strdup( "/usr/share/leiads" );

	#else

		const gchar *const *dirs;

		for ( dirs = g_get_system_data_dirs(); dirs != NULL && *dirs != NULL; dirs++ )
		{
			gchar *dirname = g_build_filename( *dirs, leiads_dir, NULL );

			ASSERT( dirname != NULL );
			if ( g_file_test( dirname, G_FILE_TEST_IS_DIR ) )
			{
				share_dirname = dirname;
				break;
			}

			g_free( dirname );
		}

		if ( share_dirname == NULL ) share_dirname = g_strdup( "." );

	#endif

		ASSERT( share_dirname != NULL );
	}

	return g_build_filename( share_dirname, filename, NULL );
}

/*-----------------------------------------------------------------------------------------------*/

static gint append_notebook_page( GtkNotebook *notebook, GtkWidget *child, const gchar *str )
{
	return gtk_notebook_append_page( notebook, child, GTK_WIDGET(gtk_label_new( str )) );
}

void Customise( GtkMenuItem *menu_item, gpointer user_data )
{
	struct settings_widgets widgets;
	GtkDialog *dialog;
	GtkNotebook *notebook;

	TRACE(( "-> Customise()\n" ));

	memset( &widgets, 0, sizeof( widgets ) );

	dialog = new_modal_dialog_with_ok_cancel( Text(CUSTOMISE), NULL );
	gtk_dialog_set_has_separator( dialog, FALSE );

	notebook = GTK_NOTEBOOK(gtk_notebook_new());
	append_notebook_page( notebook, create_startup_customisation( &widgets.startup, &Settings->startup ), Text(STARTUP) );
	append_notebook_page( notebook, create_browser_customisation( &widgets.browser, &Settings->browser ), Text(BROWSER) );
	append_notebook_page( notebook, create_playlist_customisation( &widgets.playlist, &Settings->playlist ), Text(PLAYLIST) );
	append_notebook_page( notebook, create_now_playing_customisation( &widgets.now_playing, &Settings->now_playing ), Text(NOW_PLAYING) );
	append_notebook_page( notebook, create_volume_customisation( &widgets.volume, &Settings->volume ), Text(VOLUME) );
#if LOGLEVEL > 1
	append_notebook_page( notebook, create_keys_customisation( &widgets.keys, &Settings->keys ), Text(KEYS) );
#elif defined( MAEMO )
	append_notebook_page( notebook, create_keys_customisation( &widgets.keys, &Settings->keys ), Text(KEYS) );
#else
	if ( IsSmartQ() ) append_notebook_page( notebook, create_keys_customisation( &widgets.keys, &Settings->keys ), Text(KEYS) );
#endif

	gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(notebook) );
	gtk_widget_show_all( GTK_WIDGET(dialog) );

	if ( gtk_dialog_run( dialog ) == GTK_RESPONSE_OK )
	{
		get_startup_customisation( &widgets.startup, &Settings->startup );
		get_browser_customisation( &widgets.browser, &Settings->browser );
		get_playlist_customisation( &widgets.playlist, &Settings->playlist );
		get_now_playing_customisation( &widgets.now_playing, &Settings->now_playing );
		get_volume_customisation( &widgets.volume, &Settings->volume );
		get_keys_customisation( &widgets.keys, &Settings->keys );

		OnBrowserSettingsChanged();
		OnPlaylistSettingsChanged();
		OnNowPlayingSettingsChanged();
		OnVolumeSettingsChanged();

		SaveSettings( TRUE );
	}

	gtk_widget_destroy( GTK_WIDGET(dialog) );

	TRACE(( "<- Customise()\n" ));
}

void Options( GtkMenuItem *menu_item, gpointer user_data )
{
	const char *class_id = (const char *)user_data;
	gint current_page_num = -1, page_num;

	struct settings_widgets widgets;
	GtkDialog *dialog;
	GtkNotebook *notebook;

	TRACE(( "-> Options()\n" ));

	memset( &widgets, 0, sizeof( widgets ) );

	dialog = new_modal_dialog_with_ok_cancel( Text(OPTIONS), NULL );
	gtk_dialog_set_has_separator( dialog, FALSE );

	notebook = GTK_NOTEBOOK(gtk_notebook_new());

	page_num = append_notebook_page( notebook, create_browser_options( &widgets.browser, &Settings->browser ), Text(BROWSER) );
	if ( class_id == localId ) current_page_num = page_num;
#if RADIOTIME || RADIOTIME_LINN_DS
	page_num = append_notebook_page( notebook, create_radiotime_options( &widgets.radiotime, &Settings->radiotime ), "Radio Time" );
	if ( class_id == radiotimeId ) current_page_num = page_num;
#endif
#if LASTFM
	page_num = append_notebook_page( notebook, create_lastfm_options( &widgets.lastfm, &Settings->lastfm ), "Last.fm" );
	if ( class_id == lastfmId ) current_page_num = page_num;
#endif
	append_notebook_page( notebook, create_ssdp_options( &widgets.ssdp, &Settings->ssdp ), Text(SETTINGS_SSDP) );

	gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(notebook) );
	gtk_widget_show_all( GTK_WIDGET(dialog) );
	if ( current_page_num >= 0 ) gtk_notebook_set_current_page( notebook, current_page_num );

	if ( gtk_dialog_run( dialog ) == GTK_RESPONSE_OK )
	{
	#if RADIOTIME || RADIOTIME_LINN_DS
		get_radiotime_options( &widgets.radiotime, &Settings->radiotime );
	#endif
	#if LASTFM
		get_lastfm_options( &widgets.lastfm, &Settings->lastfm );
	#endif
		get_browser_options( &widgets.browser, &Settings->browser );
		get_ssdp_options( &widgets.ssdp, &Settings->ssdp );

		OnBrowserSettingsChanged();

		SaveSettings( TRUE );
	}

	gtk_widget_destroy( GTK_WIDGET(dialog) );

	TRACE(( "<- Options()\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

gboolean IsSmartQ( void )
{
#if !defined( MAEMO ) && !defined( WIN32 )
	const gchar *host_name = g_get_host_name();
	return ( host_name != NULL && strcmp( host_name, "SmartQ" ) == 0 );
#else
	return FALSE;
#endif
}

/*-----------------------------------------------------------------------------------------------*/

static const char user_config_filename[] = "settings.xml";

void load_settings( void )
{
	gchar *filename = BuildUserConfigFilename( user_config_filename );
	gchar *contents;

	if ( g_file_get_contents( filename, &contents, NULL, NULL ) )
	{
		char *s = contents;
		char *name;

		/* Verify that it is a valid settings file */
		if ( (s = xml_unbox( &s, &name, NULL )) != NULL && strcmp( name, "Settings" ) == 0 )
		{
			char *page, *t, *key, *value;

			while ( (t = xml_unbox( &s, &page, NULL )) != NULL )
			{
				while ( (value = xml_unbox( &t, &key, NULL )) != NULL )
				{
					TRACE(( "%s.%s := %s\n", page, key, value ));

					if ( strcmp( page, "Startup" ) == 0 )
						load_startup( &Settings->startup, key, value );
					else if ( strcmp( page, "Browser" ) == 0 )
						load_browser( &Settings->browser, key, value );
					else if ( strcmp( page, "Playlist" ) == 0 )
						load_playlist( &Settings->playlist, key, value );
					else if ( strcmp( page, "NowPlaying" ) == 0 )
						load_now_playing( &Settings->now_playing, key, value );
					else if ( strcmp( page, "Volume" ) == 0 )
						load_volume( &Settings->volume, key, value );
					else if ( strcmp( page, "Keys" ) == 0 )
						load_keys( &Settings->keys, key, value );
				#if RADIOTIME || RADIOTIME_LINN_DS
					else if ( strcmp( page, "RadioTime" ) == 0 )
						load_radiotime( &Settings->radiotime, key, value );
				#endif
				#if LASTFM
					else if ( strcmp( page, "Last.fm" ) == 0 )
						load_lastfm( &Settings->lastfm, key, value );
				#endif
					else if ( strcmp( page, "SSDP" ) == 0 )
						load_ssdp( &Settings->ssdp, key, value );
				}
			}
		}

		g_free( contents );
	}
}

gpointer save_settings( gpointer user_data )
{
	struct save_settings_data *data	= (struct save_settings_data *)user_data;
	gchar *filename = BuildUserConfigFilename( user_config_filename );
	FILE *fp;

	fp = fopen_utf8( filename, "wb" );
	if ( fp != NULL )
	{
		fputs( "<?xml version=\"1.0\" encoding=\"utf-8\"?>", fp );
		fputs( "<Settings xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">", fp );

		fputs( "<Startup>", fp );
		save_startup( fp, &Settings->startup );
		fputs( "</Startup>", fp );

		fputs( "<Browser>", fp );
		save_browser( fp, &Settings->browser );
		fputs( "</Browser>", fp );

		fputs( "<Playlist>", fp );
		save_playlist( fp, &Settings->playlist );
		fputs( "</Playlist>", fp );

		fputs( "<NowPlaying>", fp );
		save_now_playing( fp, &Settings->now_playing );
		fputs( "</NowPlaying>", fp );

		fputs( "<Volume>", fp );
		save_volume( fp, &Settings->volume );
		fputs( "</Volume>", fp );

		fputs( "<Keys>", fp );
		save_keys( fp, &Settings->keys );
		fputs( "</Keys>", fp );

	#if RADIOTIME || RADIOTIME_LINN_DS
		fputs( "<RadioTime>", fp );
		save_radiotime( fp, &Settings->radiotime );
		fputs( "</RadioTime>", fp );
	#endif

	#if LASTFM
		fputs( "<Last.fm>", fp );
		save_lastfm( fp, &Settings->lastfm );
		fputs( "</Last.fm>", fp );
	#endif

		fputs( "<SSDP>", fp );
		save_ssdp( fp, &Settings->ssdp );
		fputs( "</SSDP>", fp );

		fputs( "</Settings>", fp );
		fclose( fp );

		if ( data != NULL ) g_idle_add( data->success_function, NULL );
	}
	else
	{
		GError *error = NULL;
		g_set_error( &error, 0, 0, "%s\n\n%s = %s", Text(ERRMSG_CREATE_FILE), Text(FILE_NAME), filename );
		HandleError( error, Text(ERROR_SAVE_SETTINGS) );
	}

	g_free( data );
	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/
