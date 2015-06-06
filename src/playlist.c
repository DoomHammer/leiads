/*
	playlist.c
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

#define GRAY_COLOR           "#606060"
#define PLAYING_COLOR        "#1194D2"

/*-----------------------------------------------------------------------------------------------*/

static GtkMenuItem *mi_playlist;
static GtkWidget *mi_clear, *mi_shuffle, *mi_load, *mi_save_as;
static GtkWidget *mi_playlist_service_separator, *mi_shuffle_play, *mi_repeat_mode;
static GtkWidget *mi_refresh_separator, *mi_refresh;
static gulong id_shuffle_play, id_repeat_mode;

static GtkToolItem *ti_clear, *ti_shuffle, *ti_load, *ti_save_as;
static GtkToolItem *ti_play_track, *ti_move_track_up, *ti_move_track_down, *ti_delete_track;

static GtkTreeView *playlist;
static GtkTreeViewColumn *playlist_column;
static GtkListStore *playlist_store;
static GtkMenu *playlist_renderer_menu;
static gboolean lock_selection_changed;

int PlaylistTrackCount;

enum { COL_NUMBER = 0, COL_TITLE, COL_ARTIST, NUM_COLUMNS };

GtkTreeView *create_playlist( void );
void playlist_set_sensitive( gboolean sensitive );
void playlist_set_count_sensitive( gboolean sensitive );
void playlist_set_selection_sensitive( gboolean play_delete, gboolean move_up, gboolean move_down );
void renumber_playlist( int IndexFrom, int IndexTo );
void playlist_data_func( GtkTreeViewColumn *column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data );
void update_playlist_info( void );
void update_playlist_tracks( int IndexFrom, int IndexTo );
int  get_selected_playlist_track( gboolean lock );
void select_playlist_track( int Index, gboolean unlock );
void set_playlist_model( GtkTreeModel *model );
GtkTreeModel *unset_playlist_model( void );
gchar *playlist_name( const char *filename );

#define playlist_clear DoPlaylistClear
void playlist_shuffle( void );
gboolean shuffle_playlist( gpointer data );
void playlist_load( void );
gboolean load_playlist_part1( gpointer data );
gboolean load_playlist_part2( gpointer data );
gboolean load_playlist_end( gpointer data );
void playlist_save_as( void );
void playlist_play( void );
void playlist_play_track( int index );
void playlist_move_up( void );
void playlist_move_down( void );
void playlist_delete( void );
void playlist_shuffle_play( void );
void playlist_repeat_mode( void );
void playlist_refresh( void );

void playlist_selection_changed( GtkTreeSelection *selection /*, gpointer user_data */ );
void playlist_row_activated( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column /*, gpointer user_data */ );
void playlist_column_clicked( void );

/*-----------------------------------------------------------------------------------------------*/
#ifndef MAEMO

void playlist_drag_begin( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data );
void playlist_drag_data_get( GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data );
/*gboolean playlist_drag_failed( GtkWidget *widget, GdkDragContext *drag_context, GtkDragResult result, gpointer user_data );*/
void playlist_drag_end( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data );

#endif
/*-----------------------------------------------------------------------------------------------*/

void CreatePlaylistMenuItems( GtkMenuShell *menu )
{
	GtkMenuShell *sub_menu;

	TRACE(( "-> CreatePlaylistMenuItems()\n" ));

	mi_playlist = GTK_MENU_ITEM(gtk_menu_item_new_with_label( Text(PLAYLIST) ));
	sub_menu = GTK_MENU_SHELL(gtk_menu_new());
	gtk_menu_item_set_submenu( mi_playlist, GTK_WIDGET(sub_menu) );
	gtk_menu_shell_append( menu, GTK_WIDGET(mi_playlist) );

	mi_clear = gtk_menu_item_new_with_label( Text(PLAYLIST_CLEAR) );
	g_signal_connect( G_OBJECT(mi_clear), "activate", G_CALLBACK(playlist_clear), NULL );
	gtk_menu_shell_append( sub_menu, mi_clear );

	mi_shuffle = gtk_menu_item_new_with_label( Text(PLAYLIST_SHUFFLE) );
	g_signal_connect( G_OBJECT(mi_shuffle), "activate", G_CALLBACK(playlist_shuffle), NULL );
	gtk_menu_shell_append( sub_menu, mi_shuffle );

#ifdef PLAYLIST_FILE_SUPPORT
	gtk_menu_shell_append( sub_menu, gtk_separator_menu_item_new() );

	mi_load = gtk_menu_item_new_with_label( Text(PLAYLIST_LOAD) );
	g_signal_connect( G_OBJECT(mi_load), "activate", G_CALLBACK(playlist_load), NULL );
	gtk_menu_shell_append( sub_menu, mi_load );

	mi_save_as = gtk_menu_item_new_with_label( Text(PLAYLIST_SAVE_AS) );
	g_signal_connect( G_OBJECT(mi_save_as), "activate", G_CALLBACK(playlist_save_as), NULL );
	gtk_menu_shell_append( sub_menu, mi_save_as );
#endif

	mi_playlist_service_separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append( sub_menu, mi_playlist_service_separator );

	mi_shuffle_play = gtk_check_menu_item_new_with_label( Text(PLAYLIST_SHUFFLE_PLAY) );
	id_shuffle_play = g_signal_connect( G_OBJECT(mi_shuffle_play), "activate", G_CALLBACK(playlist_shuffle_play), NULL );
	gtk_menu_shell_append( sub_menu, mi_shuffle_play );

	mi_repeat_mode = gtk_check_menu_item_new_with_label( Text(PLAYLIST_REPEAT_MODE) );
	id_repeat_mode = g_signal_connect( G_OBJECT(mi_repeat_mode), "activate", G_CALLBACK(playlist_repeat_mode), NULL );
	gtk_menu_shell_append( sub_menu, mi_repeat_mode );

	mi_refresh_separator = gtk_separator_menu_item_new();
	gtk_menu_shell_append( sub_menu, mi_refresh_separator );

	mi_refresh = gtk_menu_item_new_with_label( Text(KEYBD_REFRESH) );
	g_signal_connect( G_OBJECT(mi_refresh), "activate", G_CALLBACK(playlist_refresh), NULL );
	gtk_menu_shell_append( sub_menu, mi_refresh );

	TRACE(( "<- CreatePlaylistMenuItems()\n" ));
}

void CreatePlaylistToolItems( GtkToolbar *toolbar )
{
	TRACE(( "-> CreatePlaylistToolItems()\n" ));

	ti_clear = gtk_tool_button_new_from_stock( GTK_STOCK_CLEAR/*CLOSE*/ );
	fix_hildon_tool_button( ti_clear );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_clear), Text(PLAYLIST_CLEAR) );*/
	gtk_tool_item_set_tooltip_text( ti_clear, Text(PLAYLIST_CLEAR) );
	g_signal_connect( G_OBJECT(ti_clear), "clicked", G_CALLBACK(playlist_clear), NULL );
	gtk_toolbar_insert( toolbar, ti_clear, -1 );

	ti_shuffle = gtk_tool_button_new_from_stock( GTK_STOCK_REFRESH/*CONVERT*/ );
	fix_hildon_tool_button( ti_shuffle );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_shuffle), Text(PLAYLIST_SHUFFLE) );*/
	gtk_tool_item_set_tooltip_text( ti_shuffle, Text(PLAYLIST_SHUFFLE) );
	g_signal_connect( G_OBJECT(ti_shuffle), "clicked", G_CALLBACK(playlist_shuffle), NULL );
	gtk_toolbar_insert( toolbar, ti_shuffle, -1 );

#ifdef PLAYLIST_FILE_SUPPORT
	gtk_toolbar_insert( toolbar, gtk_separator_tool_item_new(), -1 );

	ti_load = gtk_tool_button_new_from_stock( GTK_STOCK_OPEN );
	fix_hildon_tool_button( ti_load );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_load), Text(PLAYLIST_LOAD) );*/
	gtk_tool_item_set_tooltip_text( ti_load, Text(PLAYLIST_LOAD) );
	g_signal_connect( G_OBJECT(ti_load), "clicked", G_CALLBACK(playlist_load), NULL );
	gtk_toolbar_insert( toolbar, ti_load, -1 );

	ti_save_as = gtk_tool_button_new_from_stock( GTK_STOCK_SAVE_AS );
	fix_hildon_tool_button( ti_save_as );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_save_as), Text(PLAYLIST_SAVE_AS) );*/
	gtk_tool_item_set_tooltip_text( ti_save_as, Text(PLAYLIST_SAVE_AS) );
	g_signal_connect( G_OBJECT(ti_save_as), "clicked", G_CALLBACK(playlist_save_as), NULL );
	gtk_toolbar_insert( toolbar, ti_save_as, -1 );
#endif

	gtk_toolbar_insert( toolbar, gtk_separator_tool_item_new(), -1 );

	ti_play_track = gtk_tool_button_new_from_stock( GTK_STOCK_MEDIA_PLAY );
	fix_hildon_tool_button( ti_play_track );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_play_track), Text(PLAYLIST_PLAY) );*/
	gtk_tool_item_set_tooltip_text( ti_play_track, Text(PLAYLIST_PLAY) );
	g_signal_connect( G_OBJECT(ti_play_track), "clicked", G_CALLBACK(playlist_play), NULL );
	gtk_toolbar_insert( toolbar, ti_play_track, -1 );

	ti_move_track_up = gtk_tool_button_new_from_stock( GTK_STOCK_GO_UP );
	fix_hildon_tool_button( ti_move_track_up );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_move_track_up), Text(PLAYLIST_MOVE_UP) );*/
	gtk_tool_item_set_tooltip_text( ti_move_track_up, Text(PLAYLIST_MOVE_UP) );
	g_signal_connect( G_OBJECT(ti_move_track_up), "clicked", G_CALLBACK(playlist_move_up), NULL );
	gtk_toolbar_insert( toolbar, ti_move_track_up, -1 );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_move_track_up), FALSE );

	ti_move_track_down = gtk_tool_button_new_from_stock( GTK_STOCK_GO_DOWN );
	fix_hildon_tool_button( ti_move_track_down );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_move_track_down), Text(PLAYLIST_MOVE_DOWN) );*/
	gtk_tool_item_set_tooltip_text( ti_move_track_down, Text(PLAYLIST_MOVE_DOWN) );
	g_signal_connect( G_OBJECT(ti_move_track_down), "clicked", G_CALLBACK(playlist_move_down), NULL );
	gtk_toolbar_insert( toolbar, ti_move_track_down, -1 );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_move_track_down), FALSE );

	ti_delete_track = gtk_tool_button_new_from_stock( GTK_STOCK_REMOVE /*DELETE*/ );
	fix_hildon_tool_button( ti_delete_track );
	/*gtk_tool_button_set_label( GTK_TOOL_BUTTON(ti_delete_track), Text(PLAYLIST_DELETE) );*/
	gtk_tool_item_set_tooltip_text( ti_delete_track, Text(PLAYLIST_DELETE) );
	g_signal_connect( G_OBJECT(ti_delete_track), "clicked", G_CALLBACK(playlist_delete), NULL );
	gtk_toolbar_insert( toolbar, ti_delete_track, -1 );

	TRACE(( "<- CreatePlaylistToolItems()\n" ));
}

GtkWidget *CreatePlaylist( void )
{
	GtkWidget *scrolled_window;
	GtkToolbar *toolbar;
	GtkWidget *result;

	TRACE(( "-> CreatePlaylist()\n" ));

	playlist = create_playlist();

	scrolled_window = new_scrolled_window();
	gtk_container_add( GTK_CONTAINER( scrolled_window ), GTK_WIDGET( playlist ) );

	/* Create toolbar */
	toolbar = GTK_TOOLBAR( gtk_toolbar_new() );
	gtk_toolbar_set_orientation( toolbar, GTK_ORIENTATION_VERTICAL );
	gtk_toolbar_set_style( toolbar, GTK_TOOLBAR_ICONS /* GTK_TOOLBAR_BOTH */ );
	/*gtk_toolbar_set_tooltips( toolbar, TRUE );*/

	/* Create & insert playlist toolbar button items */
	CreatePlaylistToolItems( toolbar );

	if ( Settings->playlist.toolbar == PLAYLIST_LEFT_TOOLBAR )
	{
		GtkBox *hbox = GTK_BOX( gtk_hbox_new( FALSE, 0 ) );
		gtk_box_pack_start( hbox, GTK_WIDGET( toolbar ), FALSE, TRUE, 0 );
		gtk_box_pack_start( hbox, GTK_WIDGET( scrolled_window ), TRUE, TRUE, 0 );
		result = GTK_WIDGET( hbox );
	}
	else if ( Settings->playlist.toolbar == PLAYLIST_RIGHT_TOOLBAR )
	{
		GtkBox *hbox = GTK_BOX( gtk_hbox_new( FALSE, 0 ) );
		gtk_box_pack_start( hbox, scrolled_window, TRUE, TRUE, 0 );
		gtk_box_pack_start( hbox, GTK_WIDGET( toolbar ), FALSE, TRUE, 0 );
		result = GTK_WIDGET( hbox );
	}
	else
	{
		result = scrolled_window;
	}

	TRACE(( "<- CreatePlaylist()\n" ));
	return result;
}

int OpenPlaylist( void )
{
	GtkTreeViewColumn *column = playlist_column;
	const upnp_device *sr = GetAVTransport();

	TRACE(( "-> OpenPlaylist()\n" ));
	ASSERT( sr != NULL );

	gtk_tree_view_column_set_title( column, sr->friendlyName );
	gtk_tree_view_column_set_clickable( column, playlist_renderer_menu != NULL );

	lock_selection_changed = FALSE;
	playlist_set_sensitive( FALSE );

	if ( CanShuffle() )
	{
	#ifdef PLAYLIST_REPEAT_SUPPORT
		gtk_widget_show( mi_shuffle_play );
	#else
		gtk_widget_show( mi_playlist_service_separator );
		gtk_widget_show( mi_shuffle_play );
		gtk_widget_show( mi_repeat_mode );
	#endif
	}
	else
	{
	#ifdef PLAYLIST_REPEAT_SUPPORT
		gtk_widget_hide( mi_shuffle_play );
	#else
		gtk_widget_hide( mi_playlist_service_separator );
		gtk_widget_hide( mi_shuffle_play );
		gtk_widget_hide( mi_repeat_mode );
	#endif
	}

	if ( upnp_get_ds_compatibility_family( GetAVTransport() ) != NULL )
	{
		gtk_widget_show( mi_refresh_separator );
		gtk_widget_show( mi_refresh );
	}
	else
	{
		gtk_widget_hide( mi_refresh_separator );
		gtk_widget_hide( mi_refresh );
	}

	TRACE(( "<- OpenPlaylist()\n" ));
	return 0;
}

void ClosePlaylist( void )
{
	GtkTreeViewColumn *column = playlist_column;

	TRACE(( "-> ClosePlaylist()\n" ));

	OnPlaylistClear( NULL );
	playlist_set_sensitive( FALSE );

	gtk_tree_view_column_set_clickable( column, FALSE );
	gtk_tree_view_column_set_title( column, "" );

	TRACE(( "<- ClosePlaylist()\n" ));
}

gboolean PlaylistIsOpen( void )
{
	return GTK_WIDGET_SENSITIVE( GTK_WIDGET(mi_playlist) );
}

void OnSelectPlaylist( void )
{
	TRACE(( "OnSelectPlaylist()\n" ));
}

void OnUnselectPlaylist( void )
{
	TRACE(( "OnUnselectPlaylist()\n" ));
}

/* Workaround for bug in Maemo4: Focus should not leave playlist (and enter playlist toolbar) */
static void playlist_block_keybd( int delta, gboolean long_press )
{
	TRACE(( "# playlist_block_keybd()\n" ));
	;  /* nothing to do */
}

static struct keybd_function playlist_block_keybd_function =
	{ (void (*)( int delta, gboolean long_press ))playlist_block_keybd, SUPPRESS_REPEAT };
struct keybd_function playlist_refresh_function =
	{ (void (*)( int delta, gboolean long_press ))playlist_refresh, SUPPRESS_REPEAT };

void OnPlaylistKey( GdkEventKey *event, struct keybd_function **func )
{
	if ( Settings->keys.use_system_keys ) switch ( event->keyval )
	{
	case GDK_Up:
	case GDK_Page_Up:
		*func = GTK_WIDGET_SENSITIVE( ti_move_track_up ) ? NULL : &playlist_block_keybd_function;
		break;

	case GDK_Down:
	case GDK_Page_Down:
		*func = GTK_WIDGET_SENSITIVE( ti_move_track_down ) ? NULL : &playlist_block_keybd_function;
		break;

	case GDK_space:
	case GDK_KP_Space:
	case GDK_Return:
	case GDK_KP_Enter:
		*func = NULL;
		break;
	}

	switch ( event->keyval )
	{
	case GDK_F5:
		if ( PlaylistIsOpen() && GTK_WIDGET_SENSITIVE( GTK_WIDGET(mi_playlist) ) )
			*func = &playlist_refresh_function;
		break;
	}
}

void OnPlaylistSettingsChanged( void )
{
	;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean ClearPlaylist( void )
{
	GError *error = NULL;
	gboolean result;

	TRACE(( "-> ClearPlaylist()\n" ));

	busy_enter();

	if ( !IsStopped() ) Stop( NULL );

	result = PlaylistClear( NULL, &error );
	HandleError( error, Text(ERROR_PLAYLIST_CLEAR) );

	busy_leave();

	TRACE(( "<- ClearPlaylist() = %s\n", (result) ? "TRUE" : "FALSE" ));
	return result;
}

gboolean AddToPlaylist( char *playlist_data, const gchar *title )
{
	int track_count = GetTrackCount();
	GError *error = NULL;
	gboolean result;

	TRACE(( "-> AddToPlaylist( data = %p, title = \"%s\" )\n", playlist_data, (title != NULL) ? title : "<NULL>" ));

	busy_enter();

	result = PlaylistAdd( playlist_data, NULL, &error );
	TRACE(( "PlaylistAdd() = %s\n", (result) ? "TRUE" : "FALSE" ));
	if ( result )
	{
		if ( title != NULL )
		{
			show_information(
				(track_count > 0) ? GTK_STOCK_ADD : GTK_STOCK_APPLY,
				(track_count > 0) ? Text(PLAYLIST_ADDED) : Text(PLAYLIST_REPLACED),
				title );
		}

		if ( track_count == 0 )
		{
			TRACE(( "AddToPlaylist(): StartPlaying()...\n" ));
			StartPlaying();
		}
	}

	HandleError( error, Text(ERROR_PLAYLIST_ADD) );
	busy_leave();

	TRACE(( "<- AddToPlaylist() = %s\n", (result) ? "TRUE" : "FALSE" ));
	return result;
}

/*-----------------------------------------------------------------------------------------------*/

void OnPlaylistClear( gpointer user_data )
{
	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	TRACE(( "-> OnPlaylistClear()\n" ));

	playlist_set_count_sensitive( FALSE );
	gtk_list_store_clear( playlist_store );
	tree_view_queue_resize( playlist );

	PlaylistTrackCount = 0;
	playlist_set_sensitive( TRUE );

	TRACE(( "<- OnPlaylistClear()\n" ));

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

void OnPlaylistInsert( int Index, int Count, gpointer user_data )
{
	int SelectedIndex, i;
	GtkTreeModel *model;

	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	TRACE(( "-> OnPlaylistInsert( Index = %d, Count = %d )\n", Index, Count ));

	busy_enter();

	SelectedIndex = get_selected_playlist_track( TRUE );

	model = ( PlaylistTrackCount == 0 ) ? unset_playlist_model() : NULL;

	for ( i = Index; i < Index + Count; i++ )
	{
		gchar *item = GetTrackItem( i );
		gchar *title, *artist;
		GtkTreeIter iter;
		char number[8];

		sprintf( number, "%d. ", 1 + i );

		title = GetTitle( item, NULL, 0 );
		artist = GetArtist( item, NULL, 0 );

		if ( artist != NULL )
		{
			gchar *tmp = g_strconcat( title, "  ", NULL );
			g_free( title );
			title = tmp;
		}

		gtk_list_store_insert( playlist_store, &iter, i );
		gtk_list_store_set( playlist_store, &iter,
			COL_NUMBER, number,
			COL_TITLE, title,
			COL_ARTIST, (artist != NULL) ? artist : "",
			-1 );

		if ( artist != NULL ) g_free( artist );
		if ( title != NULL ) g_free( title );

		g_free( item );

		PlaylistTrackCount++;
		if ( SelectedIndex >= i ) SelectedIndex++;
	}

	if ( i < PlaylistTrackCount )
		renumber_playlist( i, PlaylistTrackCount-1 );

	set_playlist_model( model );

	playlist_set_count_sensitive( PlaylistTrackCount > 0 );
	select_playlist_track( SelectedIndex, TRUE );

	busy_leave();
	TRACE(( "<- OnPlaylistInsert()\n" ));

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

void OnPlaylistDelete( int Index, int Count, gpointer user_data )
{
	int SelectedIndex, i;
	GtkTreeModel *model;
	char path_string[12];
	GtkTreeIter iter;
	gboolean flag;

	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	TRACE(( "-> OnPlaylistDelete( Index = %d, Count = %d )\n", Index, Count ));

	busy_enter();

	SelectedIndex = get_selected_playlist_track( TRUE );

	model = ( PlaylistTrackCount == Count ) ? unset_playlist_model() : NULL;

	sprintf( path_string, "%d", Index );
	flag = gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( playlist_store ), &iter, path_string );
	for ( i = 0; flag && i < Count; i++ )
	{
		flag = gtk_list_store_remove( playlist_store, &iter );

		PlaylistTrackCount--;
		if ( SelectedIndex > Index ) SelectedIndex--;
	}

	if ( Index < PlaylistTrackCount )
		renumber_playlist( Index, PlaylistTrackCount-1 );

	/* Only resize columns if this even was not caused by pressing the "delete", "move up", or "move down" toolbar button
	   (Otherwise we get ugly looking playlist "flickering") */
	if ( model != NULL || Count > 1 ) tree_view_queue_resize( playlist );

	set_playlist_model( model );
	playlist_set_count_sensitive( PlaylistTrackCount > 0 );
	select_playlist_track( SelectedIndex, TRUE );

	busy_leave();
	TRACE(( "<- OnPlaylistDelete()\n" ));

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

void OnPlaylistMove( int IndexFrom, int IndexTo, int Count, gpointer user_data )
{
	char path_string[12];
	GtkTreeIter iter[2];
	int SelectedIndex;

	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	TRACE(( "-> OnPlaylistMove( Index = (%d,%d), Count = %d )\n", IndexFrom, IndexTo, Count ));

	busy_enter();

	SelectedIndex = get_selected_playlist_track( TRUE );

	if ( Count == 1 )
	{
		sprintf( path_string, "%d", IndexFrom );
		if ( gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( playlist_store ), &iter[0], path_string ) )
		{
			sprintf( path_string, "%d", IndexTo );
			if ( gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( playlist_store ), &iter[1], path_string ) )
			{
				gtk_list_store_swap( playlist_store, &iter[0], &iter[1] );

				if ( SelectedIndex == IndexFrom ) SelectedIndex = IndexTo;
				else if ( SelectedIndex == IndexTo ) SelectedIndex = IndexFrom;
			}

			renumber_playlist( IndexFrom, IndexTo );
		}
	}
	else if ( Count > 1 )
	{
		/* TODO: Use gtk_list_store_reorder() here */
		ASSERT( FALSE );
	}

	/*playlist_set_count_sensitive( PlaylistTrackCount > 0 );*/
	select_playlist_track( SelectedIndex, TRUE );

	busy_leave();
	TRACE(( "<- OnPlaylistMove()\n" ));

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

void OnPlaylistTransportState( enum TransportState transport_state )
{
	TRACE(( "-> OnPlaylistTransportState( %d )\n", transport_state ));

	update_playlist_info();
	update_playlist_tracks( CurrentTrackIndex, CurrentTrackIndex );

	TRACE(( "<- OnPlaylistTransportState()\n" ));
}

void OnPlaylistTrackIndex( int OldTrackIndex, int NewTrackIndex )
{
	TRACE(( "-> OnPlaylistTrackIndex( %d => %d )\n", OldTrackIndex, NewTrackIndex ));

	update_playlist_info();
	update_playlist_tracks( OldTrackIndex, NewTrackIndex );

	if ( IsLastfmTrack )
	{
		gtk_widget_set_sensitive( GTK_WIDGET(mi_shuffle), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET(mi_save_as), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET(mi_shuffle_play), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET(mi_repeat_mode), FALSE );

		gtk_widget_set_sensitive( GTK_WIDGET(ti_shuffle), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET(ti_save_as), FALSE );

		playlist_set_selection_sensitive( FALSE, FALSE, FALSE );
	}
	else
	{
		gtk_widget_set_sensitive( GTK_WIDGET(mi_shuffle_play), TRUE );
		gtk_widget_set_sensitive( GTK_WIDGET(mi_repeat_mode), TRUE );
	}

	TRACE(( "<- OnPlaylistTrackIndex()\n" ));
}

void OnPlaylistRepeat( gboolean is_active, gpointer user_data )
{
	ASSERT( user_data == NULL );

	TRACE(( "OnPlaylistRepeat( %s )\n", (is_active) ? "TRUE" : "FALSE" ));

	g_signal_handler_block( G_OBJECT(mi_repeat_mode), id_repeat_mode );
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_repeat_mode), is_active );
	g_signal_handler_unblock( G_OBJECT(mi_repeat_mode), id_repeat_mode );
}

void OnPlaylistShuffle( gboolean is_active, gpointer user_data )
{
	ASSERT( user_data == NULL );

	TRACE(( "OnPlaylistShuffle( %s )\n", (is_active) ? "TRUE" : "FALSE" ));

	g_signal_handler_block( G_OBJECT(mi_shuffle_play), id_shuffle_play );
	gtk_check_menu_item_set_active( GTK_CHECK_MENU_ITEM(mi_shuffle_play), is_active );
	g_signal_handler_unblock( G_OBJECT(mi_shuffle_play), id_shuffle_play );

	update_playlist_tracks( 0, CurrentTrackIndex - 1 );
}

/*-----------------------------------------------------------------------------------------------*/

GtkTreeView *create_playlist( void )
{
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkTreeView *playlist;

	TRACE(( "-> create_playlist()\n" ));
	lock_selection_changed = TRUE;

	ASSERT( NUM_COLUMNS == 3 );
	playlist_store = gtk_list_store_new( NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING );

	playlist = GTK_TREE_VIEW( gtk_tree_view_new_with_model( GTK_TREE_MODEL( playlist_store ) ) );
	gtk_tree_view_set_headers_visible( playlist, TRUE );
	g_object_unref( playlist_store );

	selection = gtk_tree_view_get_selection( playlist );
	gtk_tree_selection_set_mode( selection, GTK_SELECTION_SINGLE );
	g_signal_connect( G_OBJECT(selection), "changed", GTK_SIGNAL_FUNC(playlist_selection_changed), NULL );

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_sizing( column, GTK_TREE_VIEW_COLUMN_GROW_ONLY );

	renderer = gtk_cell_renderer_text_new();
	g_object_set( renderer, "xalign", 1.0, NULL );
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_NUMBER, NULL );
	gtk_tree_view_column_set_cell_data_func( column, renderer, playlist_data_func, NULL, NULL );

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_TITLE, NULL );
	gtk_tree_view_column_set_cell_data_func( column, renderer, playlist_data_func, NULL, NULL );

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start( column, renderer, FALSE );
	gtk_tree_view_column_set_attributes( column, renderer, "text", COL_ARTIST, NULL );
	gtk_tree_view_column_set_cell_data_func( column, renderer, playlist_data_func, NULL, NULL );

	gtk_tree_view_append_column( playlist, playlist_column = column );
	gtk_tree_selection_unselect_all( selection );

	g_signal_connect( G_OBJECT(column), "clicked", G_CALLBACK(playlist_column_clicked), NULL );
	g_signal_connect( G_OBJECT(playlist), "row-activated", GTK_SIGNAL_FUNC(playlist_row_activated), NULL );

	g_signal_connect( G_OBJECT(playlist), "key-press-event", G_CALLBACK(KeyPressEvent), GINT_TO_POINTER(PLAYLIST_PAGE) );
	g_signal_connect( G_OBJECT(playlist), "key-release-event", G_CALLBACK(KeyReleaseEvent), GINT_TO_POINTER(PLAYLIST_PAGE) );

#if 0
	{
		GdkColor color;
		gdk_color_parse( "#FFFFFF", &color );
		gtk_widget_modify_base( GTK_WIDGET(playlist), GTK_STATE_NORMAL, &color );
	}
#endif

#ifndef MAEMO
	if ( !Settings->startup.classic_ui )
	{
		gtk_drag_source_set( GTK_WIDGET(playlist), GDK_BUTTON1_MASK,
			TargetList, 1, GDK_ACTION_DEFAULT|GDK_ACTION_PRIVATE|GDK_ACTION_MOVE );

		g_signal_connect( G_OBJECT(playlist), "drag-begin", GTK_SIGNAL_FUNC(playlist_drag_begin), NULL );
		g_signal_connect( G_OBJECT(playlist), "drag-data-get", GTK_SIGNAL_FUNC(playlist_drag_data_get), NULL );
		/*g_signal_connect( G_OBJECT(playlist), "drag-failed", GTK_SIGNAL_FUNC(playlist_drag_failed), NULL );*/
		g_signal_connect( G_OBJECT(playlist), "drag-end", GTK_SIGNAL_FUNC(playlist_drag_end), NULL );

		gtk_drag_dest_set( GTK_WIDGET(playlist), GTK_DEST_DEFAULT_MOTION|GTK_DEST_DEFAULT_DROP,
			TargetList, G_N_ELEMENTS( TargetList ), GDK_ACTION_COPY|GDK_ACTION_MOVE );

		/*g_signal_connect( G_OBJECT(playlist), "drag-drop", GTK_SIGNAL_FUNC(drag_drop), NULL );*/
		g_signal_connect( G_OBJECT(playlist), "drag-data-received", GTK_SIGNAL_FUNC(drag_data_received), GUINT_TO_POINTER(GDK_ACTION_COPY) );
	}
#endif

	lock_selection_changed = FALSE;
	TRACE(( "<- create_playlist()\n" ));
	return playlist;
}

void playlist_set_sensitive( gboolean sensitive )
{
	gtk_widget_set_sensitive( GTK_WIDGET(mi_playlist), sensitive );

#ifdef PLAYLIST_FILE_SUPPORT
	gtk_widget_set_sensitive( GTK_WIDGET(mi_load), sensitive );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_load), sensitive );
#endif

	playlist_set_count_sensitive( (sensitive) ? PlaylistTrackCount > 0 : FALSE );
	playlist_set_selection_sensitive( FALSE, FALSE, FALSE );
}

void playlist_set_count_sensitive( gboolean sensitive )
{
	gtk_widget_set_sensitive( GTK_WIDGET(mi_clear), sensitive );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_clear), sensitive );

	gtk_widget_set_sensitive( GTK_WIDGET(mi_shuffle), sensitive );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_shuffle), sensitive );

#ifdef PLAYLIST_FILE_SUPPORT
	gtk_widget_set_sensitive( GTK_WIDGET(mi_save_as), sensitive );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_save_as), sensitive );
#endif
}

void playlist_set_selection_sensitive( gboolean play_delete, gboolean move_up, gboolean move_down )
{
	TRACE(( "playlist_set_selection_sensitive(): ti_play_track = %s\n", (play_delete) ? "TRUE" : "FALSE" ));
	gtk_widget_set_sensitive( GTK_WIDGET(ti_play_track), play_delete );

	TRACE(( "playlist_set_selection_sensitive(): ti_move_track_up = %s\n", (move_up) ? "TRUE" : "FALSE" ));
	gtk_widget_set_sensitive( GTK_WIDGET(ti_move_track_up), move_up );

	TRACE(( "playlist_set_selection_sensitive(): ti_move_track_down = %s\n", (move_down) ? "TRUE" : "FALSE" ));
	gtk_widget_set_sensitive( GTK_WIDGET(ti_move_track_down), move_down );

	TRACE(( "playlist_set_selection_sensitive(): ti_delete_track = %s\n", (play_delete) ? "TRUE" : "FALSE" ));
	gtk_widget_set_sensitive( GTK_WIDGET(ti_delete_track), play_delete );
}

void renumber_playlist( int IndexFrom, int IndexTo )
{
	GtkTreeIter iter;
	char buf[12];

	TRACE(( "-> renumber_playlist( from %d to %d )\n", IndexFrom, IndexTo ));

	if ( IndexTo < IndexFrom ) { int index = IndexFrom; IndexFrom = IndexTo; IndexTo = index; }
	if ( IndexFrom < 0 ) IndexFrom = 0;
	if ( IndexTo < 0 ) return;

	TRACE(( "renumber rows [%d..%d]\n", IndexFrom, IndexTo ));

	sprintf( buf, "%d", IndexFrom );
	if ( gtk_tree_model_get_iter_from_string( GTK_TREE_MODEL( playlist_store ), &iter, buf ) )
	{
		while ( IndexFrom <= IndexTo )
		{
			sprintf( buf, "%d. ", ++IndexFrom );
			gtk_list_store_set( playlist_store, &iter, COL_NUMBER, buf, -1 );

			if ( !gtk_tree_model_iter_next( GTK_TREE_MODEL( playlist_store ), &iter ) ) break;
		}
	}

	TRACE(( "<- renumber_playlist()\n" ));
}

void playlist_data_func( GtkTreeViewColumn *column, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer data )
{
	gboolean random = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(mi_shuffle_play) );
	gchar *path_string = gtk_tree_model_get_string_from_iter( model, iter );
	int track_index = atoi( path_string );

	if ( track_index < CurrentTrackIndex && !random )
	{
		g_object_set( renderer, "foreground", GRAY_COLOR, "foreground-set", TRUE, NULL );
	}
	else if ( track_index == CurrentTrackIndex && (TransportState == TRANSPORT_STATE_PLAYING || TransportState == TRANSPORT_STATE_PAUSED) )
	{
	#if 1
		g_object_set( renderer, "foreground", PLAYING_COLOR, "foreground-set", TRUE, NULL );
	#else
		const GdkColor *color = get_widget_base_color( GTK_WIDGET(playlist), GTK_STATE_SELECTED );
		gchar *color_str = gdk_color_to_string( color );
		g_object_set( renderer, "foreground", color_str, "foreground-set", TRUE, NULL );
		g_free( color_str );
	#endif
	}
	else
	{
		g_object_set( renderer, "foreground-set", FALSE, NULL );
	}
}

void update_playlist_info( void )
{
#ifndef MAEMO
	if ( !Settings->startup.classic_ui )
	{
		gchar *item = GetCurrentTrackItem();

		if ( item != NULL )
		{
			gchar *title = GetTitleAndArtist( item, NULL, 0 );

			if ( TransportState == TRANSPORT_STATE_PLAYING )
			{
				/* TODO: Add current position as " (0:12)" */
				gtk_window_set_title( MainWindow, title );
			}
			else
			{
				gchar *window_title;
				const char *state;

				if ( TransportState == TRANSPORT_STATE_TRANSITIONING )
					state = Text(TRANSPORT_STATE_TRANSITIONING);
				else if ( TransportState == TRANSPORT_STATE_BUFFERING )
					state = Text(TRANSPORT_STATE_BUFFERING);
				else if ( TransportState == TRANSPORT_STATE_PAUSED )
					state = Text(TRANSPORT_STATE_PAUSED);
				else if ( TransportState == TRANSPORT_STATE_STOPPED )
					state = Text(TRANSPORT_STATE_STOPPED);
				else
					state = "?";

				window_title = g_strdup_printf( "%s [%s]", title, state );
				gtk_window_set_title( MainWindow, window_title );
				g_free( window_title );
			}

			g_free( title );
			g_free( item );
		}
		else
		{
			gtk_window_set_title( MainWindow, NAME );
		}
	}
#endif
}

void update_playlist_tracks( int IndexFrom, int IndexTo )
{
	TRACE(( "-> update_playlist_tracks( from %d to %d )\n", IndexFrom, IndexTo ));

	if ( IndexTo < IndexFrom ) { int index = IndexFrom; IndexFrom = IndexTo; IndexTo = index; }
	if ( IndexFrom < 0 ) IndexFrom = 0;
	if ( IndexTo < 0 ) return;

	TRACE(( "redraw rows [%d..%d]\n", IndexFrom, IndexTo ));
{
#if 0

	GtkTreeViewColumn *column = playlist_column;
	GdkRectangle rect1, rect2;
	GtkTreePath *path;

	ASSERT( FromIndex >= 0 && ToIndex >= 0 );

	path = gtk_tree_path_new_from_indices( FromIndex, -1 );
	gtk_tree_view_get_cell_area( playlist, path, column, &rect1 );
	gtk_tree_path_free( path );
	TRACE(( "rect1: %d,%d,%d,%d\n", rect1.x, rect1.y, rect1.width, rect1.height ));

	path = gtk_tree_path_new_from_indices( ToIndex, -1 );
	gtk_tree_view_get_cell_area( playlist, path, column, &rect2 );
	gtk_tree_path_free( path );
	TRACE(( "rect2: %d,%d,%d,%d\n", rect2.x, rect2.y, rect2.width, rect2.height ));

	rect1.height += rect2.y - rect1.y;
	TRACE(( "rect: %d,%d,%d,%d\n", rect1.x, rect1.y, rect1.width, rect1.height ));

	gtk_tree_view_convert_tree_to_widget_coords( playlist, rect1.x, rect1.y, &rect1.x, &rect1.y );
	TRACE(( "rect: %d,%d,%d,%d\n", rect1.x, rect1.y, rect1.width, rect1.height ));

	gdk_window_invalidate_rect( gtk_widget_get_window( GTK_WIDGET(playlist) ), &rect1, TRUE );

#else

	while ( IndexFrom <= IndexTo )
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices( IndexFrom++, -1 );
		if ( path != NULL )
		{
			GtkTreeModel *model = GTK_TREE_MODEL(playlist_store);
			GtkTreeIter iter;

			if ( gtk_tree_model_get_iter( model, &iter, path ) )
				gtk_tree_model_row_changed( model, path, &iter );

			gtk_tree_path_free( path );
		}
	}

#endif
}

	TRACE(( "<- update_playlist_tracks()\n" ));
}

int get_selected_playlist_track( gboolean lock )
{
	GtkTreeSelection *selection = gtk_tree_view_get_selection( playlist );
	GtkTreeModel *model;
	GtkTreeIter iter;
	int Index = -1;

	if ( lock ) lock_selection_changed = TRUE;

	if ( gtk_tree_selection_get_selected( selection, &model, &iter ) )
	{
		gchar *string_path = gtk_tree_model_get_string_from_iter( model, &iter );
		if ( string_path != NULL )
		{
			Index = atoi( string_path );
			g_free( string_path );
		}
	}

	return Index;
}

void select_playlist_track( int Index, gboolean unlock )
{
	if ( Index >= 0 && Index < PlaylistTrackCount )
	{
		GtkTreePath *path = gtk_tree_path_new_from_indices( Index, -1 );
		if ( path != NULL )
		{
			GtkTreePath *start_path, *end_path;

			/* Make selection */
			gtk_tree_view_set_cursor( playlist, path, NULL, FALSE );

			/* Make sure the selection is visible */
			/* Unfortunately gtk_tree_view_scroll_to_cell( playlist, path, NULL, FALSE, 0.0, 0.0 )
			   does not work as expected, so we have to do the alignment for ourself... */
			if ( gtk_tree_view_get_visible_range( playlist, &start_path, &end_path ) )
			{
				gint *start_index = gtk_tree_path_get_indices( start_path );
				gint *end_index = gtk_tree_path_get_indices( end_path );

				if ( start_index != NULL && Index <= *start_index )
					gtk_tree_view_scroll_to_cell( playlist, path, NULL, TRUE, 0.0, 0.0 );
				else if ( end_index != NULL && Index >= *end_index )
					gtk_tree_view_scroll_to_cell( playlist, path, NULL, TRUE, 1.0, 0.0 );

				gtk_tree_path_free( end_path );
				gtk_tree_path_free( start_path );
			}

			gtk_tree_path_free( path );
		}
	}
	else
	{
		gtk_tree_selection_unselect_all( gtk_tree_view_get_selection( playlist ) );
	}

	if ( unlock )
	{
		lock_selection_changed = FALSE;
		playlist_selection_changed( NULL );
	}
}

void set_playlist_model( GtkTreeModel *model )
{
	if ( model != NULL )
	{
		gtk_tree_view_set_model( playlist, model );
		g_object_unref( model );

		gtk_tree_view_set_enable_search( playlist, TRUE );
		gtk_tree_view_set_search_column( playlist, COL_TITLE );
	}
}

GtkTreeModel *unset_playlist_model( void )
{
	GtkTreeModel *model = gtk_tree_view_get_model( playlist );

	if ( model != NULL )
	{
		g_object_ref( playlist_store );
		gtk_tree_view_set_model( playlist, NULL );
	}

	return model;
}

gchar *playlist_name( const char *filename )
{
	gchar *name, *s;

	if ( filename != NULL )  /* Load playlist from file */
	{
		name = g_path_get_basename( filename );
		s = strrchr( name, '.' );
		if ( s != NULL ) *s = '\0';
	}
	else                     /* Shuffle playlist */
		name = NULL;

	return name;
}

/*-----------------------------------------------------------------------------------------------*/

/* Callback for "Clear all" menu entry */
void playlist_clear( void )
{
	gint response;

	TRACE(( "## playlist_clear()\n" ));

	response = ( IsStopped() )
		? GTK_RESPONSE_OK
		: message_dialog( GTK_MESSAGE_QUESTION, Text(PLAYLIST_CLEAR_QUESTION) );

	if ( response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES )
	{
		busy_enter();

		if ( ClearPlaylist() )
		{
			show_information( NULL, NULL, Text(PLAYLIST_CLEARED) );
		}

		busy_leave();
	}
}

/* Callback for "Play" toolbar item */
void playlist_play( void )
{
	int index;

	index = get_selected_playlist_track( FALSE );
	TRACE(( "## playlist_play(): index = %d\n", index ));

	playlist_play_track( index );
}

void playlist_play_track( int index )
{
	if ( index >= 0 && index < PlaylistTrackCount )
	{
		GError *error = NULL;
		gchar *sk;

		if ( index <= CurrentTrackIndex && (sk = GetLastfmSessionKeyFromTrack( index )) != NULL )
		{
			g_free( sk );
			g_set_error( &error, 0, 0, Text(ERRMSG_LASTFM_RADIO) );
		}
		else
		{
			gboolean success;

			busy_enter();
			success = SeekTracksAbsolute( index, &error );
			TRACE(( "SeekTracksAbsolute() = %s\n", (success) ? "TRUE" : "FALSE" ));
			busy_leave();
		}

		HandleError( error, Text(ERROR_SEEK_TRACK) );
	}
}

/* Callback for "Move Up" toolbar item */
void playlist_move_up( void )
{
	GError *error = NULL;
	int index;

	busy_enter();

	index = get_selected_playlist_track( FALSE );
	TRACE(( "## playlist_move_up(): index = %d\n", index ));

	if ( index > 0 && index < PlaylistTrackCount )
	{
		PlaylistMove( index, index - 1, 1, NULL, &error );
		HandleError( error, Text(ERROR_PLAYLIST_MOVE) );
	}

	busy_leave();
}

/* Callback for "Move Down" toolbar item */
void playlist_move_down( void )
{
	GError *error = NULL;
	int index;

	busy_enter();

	index = get_selected_playlist_track( FALSE );
	TRACE(( "## playlist_move_down(): index = %d\n", index ));

	if ( index >= 0 && index < PlaylistTrackCount-1 )
	{
		PlaylistMove( index, index + 1, 1, NULL, &error );
		HandleError( error, Text(ERROR_PLAYLIST_MOVE) );
	}

	busy_leave();
}

/* Callback for "Delete" toolbar item */
void playlist_delete( void )
{
	GError *error = NULL;
	int index;

	index = get_selected_playlist_track( FALSE );
	TRACE(( "## playlist_delete(): index = %d\n", index ));

	if ( index >= 0 && index < PlaylistTrackCount )
	{
		gint response = ( IsStopped() || index != CurrentTrackIndex )
			? GTK_RESPONSE_OK
			: message_dialog( GTK_MESSAGE_QUESTION, Text(PLAYLIST_DELETE_QUESTION) );

		if ( response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES )
		{
			busy_enter();

			PlaylistDelete( index, 1, NULL, &error );
			HandleError( error, Text(ERROR_PLAYLIST_DELETE) );

			busy_leave();
		}
	}
}

void playlist_shuffle_play( void )
{
	gboolean is_active = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(mi_shuffle_play) );
	GError *error = NULL;

	TRACE(( "## playlist_shuffle_play(): is_active = %s\n", (is_active) ? "TRUE" : "FALSE" ));

	if ( !SetShuffle( is_active, &error ) )
	{
		OnPlaylistShuffle( ShufflePlay, NULL );
		HandleError( error, Text(ERROR_PLAYLIST_SHUFFLE) );
	}
}

void playlist_repeat_mode( void )
{
	gboolean is_active = gtk_check_menu_item_get_active( GTK_CHECK_MENU_ITEM(mi_repeat_mode) );
	GError *error = NULL;

	TRACE(( "## playlist_repeat_mode(): is_active = %s\n", (is_active) ? "TRUE" : "FALSE" ));

	if ( !SetRepeat( is_active, &error ) )
	{
		OnPlaylistRepeat( RepeatMode, NULL );
		HandleError( error, Text(ERROR_PLAYLIST_REPEAT_MODE) );
	}
}

void playlist_refresh( void )
{
	TRACE(( "## playlist_refresh()\n" ));

	if ( upnp_get_ds_compatibility_family( GetAVTransport() ) != NULL )
	{
		SetRenderer( GetRenderer(), TRUE );
	}
}

/*-----------------------------------------------------------------------------------------------*/

struct load_struct
{
	struct animation *animation;
	gchar *filename;
	struct xml_content *media_string;
	int tracks_not_added;
	GtkTreeModel *tree_model;
};

static void load_playlist( const char *action, gchar *filename, GSourceFunc function )
{
	struct load_struct *ls = g_new0( struct load_struct, 1 );
	gchar *name = playlist_name( filename );

	ls->animation     = show_animation( action, name, 0 );
	ls->filename      = filename;

	if ( name != NULL ) g_free( name );

	playlist_set_sensitive( FALSE );
	g_idle_add( function, ls );
}

/* Callback for "Shuffle" menu entry */
void playlist_shuffle( void )
{
	gint response = message_dialog( GTK_MESSAGE_QUESTION, Text(PLAYLIST_SHUFFLE_QUESTION) );
	if ( response == GTK_RESPONSE_OK || response == GTK_RESPONSE_YES )
	{
		load_playlist( Text(PLAYLIST_SHUFFLING), NULL, shuffle_playlist );
	}
}

#define LCG( X ) (1103515245 * (X) + 12345)  /* See http://en.wikipedia.org/wiki/Linear_congruential_generator */

/* Shuffle: Taken from http://en.wikipedia.org/wiki/Fisher-Yates_shuffle */
static void fisher_yates_shuffle( int *array, int array_length )
{
	unsigned X = (unsigned)time( NULL );  /* i.e., java.util.Random. */
	int n = array_length;        /* The number of items left to shuffle (loop invariant). */

	while ( n > 1 )
	{
		int k, temp;

		k = (int)(((X = LCG( X )) >> 16) % n);  /* 0 <= k < n. */
		n--;                     /* n is now the last pertinent index; */
		temp = array[n];         /* swap array[n] with array[k] (does nothing if k == n). */
		array[n] = array[k];
		array[k] = temp;
	}
}

gboolean shuffle_playlist( gpointer data )
{
	struct load_struct *ls = (struct load_struct*)data;
	GString *media_string;
	int *a, n, i;

	n = GetTrackCount();
	a = g_new( int, n );
	for ( i = 0; i < n; i++ ) a[i] = i;
	fisher_yates_shuffle( a, n );

	media_string = g_string_sized_new( n * 1024 );
	if ( media_string != NULL )
	{
		for ( i = 0; i < n; i++ )
		{
			gchar *item = GetTrackItem( a[i] );
			if ( item != NULL )
			{
				TRACE(( "strlen( item ) = %lu\n", (unsigned long)strlen( item ) ));
				g_string_append( media_string, item );

				g_free( item );
			}
		}

		ls->media_string = xml_content_from_string( media_string, n );
	}
	else
	{
		GError *error = NULL;
		g_set_error( &error, 0, 0, Text(ERRMSG_OUT_OF_MEMORY) );
		HandleError( error, Text(ERROR_PLAYLIST_SHUFFLE) );
	}

	g_free( a );

	g_idle_add( (ls->media_string != NULL) ? load_playlist_part2 : load_playlist_end, data );
	return FALSE;
}

/* Callback for "Load..." playlist menu entry */
void playlist_load( void )  /* (GtkMenuItem *menuitem, gpointer user_data) */
{
	gchar *current_playlist_folder = Settings->playlist.current_folder;
	gchar *filename = ChoosePlaylistFile( NULL, NULL, &current_playlist_folder );

	if ( filename != NULL )
	{
		load_playlist( Text(PLAYLIST_LOADING), filename, load_playlist_part1 );

		if ( Settings->playlist.current_folder != current_playlist_folder )
		{
			Settings->playlist.current_folder = current_playlist_folder;
			SaveSettings( FALSE );
		}
	}
}

gboolean load_playlist_part1( gpointer data )
{
	struct load_struct *ls = (struct load_struct*)data;
	GError *error = NULL;

	/* Load playlist file */
	ls->media_string = LoadPlaylist( ls->filename, &ls->tracks_not_added, &error );
	g_idle_add( ( ls->media_string != NULL ) ? load_playlist_part2 : load_playlist_end, data );

	HandleError( error, Text(ERROR_PLAYLIST_LOAD) );
	return FALSE;
}

gboolean load_playlist_part2( gpointer data )
{
	struct load_struct *ls = (struct load_struct*)data;
	char *playlist_data;

	ASSERT( ls->media_string != NULL );
	playlist_data = (ls->media_string != NULL) ? ls->media_string->str : NULL;

	ASSERT( playlist_data != NULL );
	if ( playlist_data != NULL )
	{
		GError *error = NULL;

		if ( !IsStopped() ) Stop( &error );
		if ( error == NULL && PlaylistClear( ENTER_GDK_THREADS, &error ) )
		{
			gdk_threads_enter();
			ls->tree_model = unset_playlist_model();
			gdk_threads_leave();

			PlaylistAdd( playlist_data, ENTER_GDK_THREADS, &error );
		}

		HandleError( error, Text(ERROR_PLAYLIST_LOAD) );
	}
	else
	{
		gdk_threads_enter();
		message_dialog( GTK_MESSAGE_ERROR, Text(PLAYLIST_NOT_A_VALID_FILE) );
		gdk_threads_leave();
	}

	g_idle_add( load_playlist_end, data );
	return FALSE;
}

gboolean load_playlist_end( gpointer data )
{
	struct load_struct *ls = (struct load_struct*)data;

	free_xml_content( ls->media_string );

	gdk_threads_enter();

	set_playlist_model( ls->tree_model );
	playlist_set_sensitive( TRUE );

	clear_animation( ls->animation );
	g_free( ls->filename );

	gdk_threads_leave();

	HandleNotAddedTracks( ls->tracks_not_added );

	g_free( ls );
	return FALSE;
}

/* Callback for "Save as..." playlist menu entry */
void playlist_save_as( void )
{
	gchar *current_playlist_folder = Settings->playlist.current_folder;
	gchar *filename;

	static int show_warning = TRUE;
	if ( show_warning )
	{
		show_warning = FALSE;
		message_dialog( GTK_MESSAGE_WARNING, Text(PLAYLIST_INFO) );
	}

	filename = ChoosePlaylistFile( NULL, Text(PLAYLIST), &current_playlist_folder );
	if ( filename != NULL )
	{
		GError *error = NULL;

		busy_enter();

		/* TODO: Use CreatePlaylist(), ... ClosePlaylist() instead */
		if ( SavePlaylist( filename, &error ) )
		{
			gchar *name, *info_text;

			name = playlist_name( filename );
			info_text = g_strdup_printf( Text(PLAYLIST_SAVED), name );
			show_information( NULL, NULL, info_text );
			g_free( info_text );
			g_free( name );
		}

		if ( Settings->playlist.current_folder != current_playlist_folder )
		{
			Settings->playlist.current_folder = current_playlist_folder;
			SaveSettings( FALSE );
		}

		busy_leave();

		g_free( filename );

		HandleError( error, Text(ERROR_PLAYLIST_SAVE), filename );
	}
}

/*-----------------------------------------------------------------------------------------------*/

void playlist_selection_changed( GtkTreeSelection *selection )  /* gpointer user_data */
{
	TRACE(( "## playlist_selection_changed(): lock=%s\n", (lock_selection_changed) ? "TRUE" : "FALSE" ));

	if ( !lock_selection_changed )
	{
		int Index = get_selected_playlist_track( FALSE );
		if ( Index >= 0 && !IsLastfmTrack )
			playlist_set_selection_sensitive( TRUE, Index > 0, Index < PlaylistTrackCount-1 );
		else
			playlist_set_selection_sensitive( FALSE, FALSE, FALSE );
	}
}

void playlist_row_activated( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column )
{
	GtkTreeModel *model = GTK_TREE_MODEL(playlist_store);
	GtkTreeIter iter;

	TRACE(( "## playlist_row_activated()\n" ));

	if ( !row_activated_flag && gtk_tree_model_get_iter( model, &iter, path ) )
	{
		gchar *path_str = gtk_tree_path_to_string( path );

		TRACE(( "gtk_tree_model_get_iter(): path = \"%s\"\n", gtk_tree_path_to_string( path ) ));
		set_row_activated_flag();

		if ( path_str != NULL )
		{
			playlist_play_track( atoi( path_str ) );
			g_free( path_str );
		}
	}
}

/*-----------------------------------------------------------------------------------------------*/

void SetPlaylistRendererMenu( GtkMenu *menu )
{
	GtkTreeViewColumn *column = playlist_column;

	if ( menu != NULL )
	{
		gtk_tree_view_column_set_clickable( column, TRUE );
		gtk_menu_attach_to_widget( menu, GTK_WIDGET(playlist), NULL );
		gtk_widget_show_all( GTK_WIDGET(menu) );
	}
	else
	{
		gtk_tree_view_column_set_clickable( column, FALSE );
		if ( playlist_renderer_menu != NULL )
		{
			gtk_menu_detach( playlist_renderer_menu );
			/*g_object_unref( playlist_renderer_menu );*/
		}
	}

	playlist_renderer_menu = menu;
}

void playlist_column_clicked( void )
{
	if ( playlist_renderer_menu != NULL )
	{
		gtk_menu_attach_to_widget( playlist_renderer_menu, GTK_WIDGET(playlist), NULL );
		gtk_menu_popup( playlist_renderer_menu, NULL, NULL, NULL, NULL,
			0, gtk_get_current_event_time() );
	}
}

/*-----------------------------------------------------------------------------------------------*/
#ifndef MAEMO

extern char **ItemArray;  /* aus media_renderer.c */

static gboolean drag_begin_func( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data )
{
	GtkWidget *widget = (GtkWidget *)user_data;

	const char *item = ItemArray[atoi( path_str )];
	if ( item != NULL )
	{
		gtk_drag_source_set_icon_stock( widget, get_upnp_class_id( item ) );
		return FALSE;
	}

	return TRUE;
}

void playlist_drag_begin( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data )
{
	TRACE(( "playlist_drag_begin()\n" ));
	foreach_selected_tree_view_row( playlist, drag_begin_func, widget );
}

static gboolean drag_data_get_func( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data )
{
	GtkSelectionData *selection_data = (GtkSelectionData *)user_data;

	gtk_selection_data_set_text( selection_data, path_str, -1 );
	return FALSE;
}

void playlist_drag_data_get( GtkWidget *widget, GdkDragContext *drag_context, GtkSelectionData *selection_data, guint info, guint time, gpointer user_data )
{
	TRACE(( "playlist_drag_data_get()\n" ));
	foreach_selected_tree_view_row( playlist, drag_data_get_func, selection_data );
}

/*gboolean playlist_drag_failed( GtkWidget *widget, GdkDragContext *drag_context, GtkDragResult result, gpointer user_data )
{
	TRACE(( "playlist_drag_failed()\n" ));
	return FALSE;
}*/

void playlist_drag_end( GtkWidget *widget, GdkDragContext *drag_context, gpointer user_data )
{
	TRACE(( "playlist_drag_end()\n" ));
	;
}

#endif
/*-----------------------------------------------------------------------------------------------*/
