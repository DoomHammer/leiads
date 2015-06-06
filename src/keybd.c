/*
	keybd.c
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

/*-----------------------------------------------------------------------------------------------*/

void keybd_nop( int delta, gboolean long_press )
{
}

void keybd_fullscreen( int delta, gboolean long_press )
{
	SetFullscreen( !GetFullscreen() );
}

void keybd_page( int delta, gboolean long_press )
{
	int page_num = GetCurrentPage();

	if ( long_press )
	{
		page_num = (delta < 0) ? 0 : NUM_PAGES-1;
	}
	else
	{
		page_num += delta;
		if ( page_num < 0 ) page_num += NUM_PAGES;
		else if ( page_num >= NUM_PAGES ) page_num -= NUM_PAGES;
	}

	SetCurrentPage( page_num );
}

void keybd_play_pause( int delta, gboolean long_press )
{
	if ( GetAVTransport() != NULL )
	{
		if ( long_press )
		{
			if ( IsStopped() )
				DoPlay();
			else
				DoStop();
		}
		else if ( IsLastfmTrack )
		{
			if ( GetRenderingControl() != NULL )
				ToggleMute();
		}
		else
		{
			if ( IsPlaying() )
				DoPause();
			else
				DoPlay();
		}
	}
}

void keybd_track( int delta, gboolean long_press )
{
	if ( GetAVTransport() != NULL )
	{
		if ( delta >= 0 )
			DoNextTrack();
		else
			DoPreviousTrack();
	}
}

void keydb_mute( int delta, gboolean long_press )
{
	if ( GetRenderingControl() != NULL ) ToggleMute();
}

void keybd_volume( int delta, gboolean long_press )
{
	if ( GetRenderingControl() != NULL )
	{
	/*	if ( delta < 0 && long_press ) volumebar_set_mute( TRUE );
		else*/ SetVolumeDelta( delta );
	}
}

static GtkDialog *dialog;

static void button_clicked( GtkButton *button, gpointer user_data )
{
	gint response_id = GPOINTER_TO_INT( user_data );

	TRACE(( "# button_clicked( %d )\n", response_id ));
	gtk_dialog_response( dialog, response_id );
}

static GtkWidget *attach_button_to_table( GtkTable *table, int count, const char *label, const char *stock_id, gint response_id )
{
	guint left_attach = count % 2;
	guint top_attach = count / 2;

	GtkWidget *button = gtk_button_new_with_label( label );
	gtk_button_set_image( GTK_BUTTON(button), gtk_image_new_from_stock( stock_id, GTK_ICON_SIZE_LARGE_TOOLBAR ) );
	gtk_widget_set_visible( gtk_button_get_image( GTK_BUTTON(button) ), TRUE );
	gtk_button_set_alignment( GTK_BUTTON(button), 0.0, 0.5 );

	g_signal_connect( G_OBJECT(button), "clicked", G_CALLBACK(button_clicked), GINT_TO_POINTER(response_id) );
	gtk_table_attach_defaults( table, button, left_attach, left_attach + 1, top_attach, top_attach + 1 );

	return button;
}

void keybd_selection( int delta, gboolean long_press )
{
	extern void handle_transport_state( int media_event, const char *state, gpointer user_data );

	int count = 0, response_id;
	guint rows;

	GtkTable *table;
	gchar *label;

	dialog = GTK_DIALOG(gtk_dialog_new_with_buttons( Text(KEYBD_SELECTION),
		GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL | GTK_DIALOG_NO_SEPARATOR, NULL ));

	rows = 1;
	if ( Settings->startup.classic_ui ) rows++;
	if ( GetAVTransport() != NULL ) rows += 3;
	if ( GetRenderingControl() != NULL ) rows += 1;

	table = GTK_TABLE(gtk_table_new( rows, 2, TRUE ));
	gtk_container_set_border_width( GTK_CONTAINER(table), BORDER );
	gtk_table_set_row_spacings( table, 2 * SPACING );
	gtk_table_set_col_spacings( table, 2 * SPACING );

	if ( Settings->startup.classic_ui )
	{
		int page_num = GetCurrentPage();

		if ( page_num != BROWSER_PAGE )
			attach_button_to_table( table, count++, Text(BROWSER), LEIADS_STOCK_BROWSER, TextIndex(BROWSER) );
		if ( page_num != PLAYLIST_PAGE )
			attach_button_to_table( table, count++, Text(PLAYLIST), LEIADS_STOCK_PLAYLIST, TextIndex(PLAYLIST) );
		if ( page_num != NOW_PLAYING_PAGE )
			attach_button_to_table( table, count++, Text(NOW_PLAYING), LEIADS_STOCK_NOW_PLAYING, TextIndex(NOW_PLAYING) );
	}

	if ( GetAVTransport() != NULL )
	{
		if ( !IsPlaying() )
			attach_button_to_table( table, count++, Text(MEDIA_PLAY), GTK_STOCK_MEDIA_PLAY, TextIndex(MEDIA_PLAY) );
		if ( IsPlaying() || IsStopped() )
			attach_button_to_table( table, count++, Text(MEDIA_PAUSE), GTK_STOCK_MEDIA_PAUSE, TextIndex(MEDIA_PAUSE) );
		if ( !IsStopped() )
			attach_button_to_table( table, count++, Text(MEDIA_STOP), GTK_STOCK_MEDIA_STOP, TextIndex(MEDIA_STOP) );

		attach_button_to_table( table, count++, Text(MEDIA_PREVIOUS), GTK_STOCK_MEDIA_PREVIOUS, TextIndex(MEDIA_PREVIOUS) );
		attach_button_to_table( table, count++, Text(MEDIA_NEXT), GTK_STOCK_MEDIA_NEXT, TextIndex(MEDIA_NEXT) );
	}

	if ( GetRenderingControl() != NULL )
	{
		attach_button_to_table( table, count++, Text(TOGGLE_MUTE), STOCK_AUDIO_VOLUME_MUTED, TextIndex(TOGGLE_MUTE) );
		attach_button_to_table( table, count++, Text(QUIET), STOCK_AUDIO_VOLUME_LOW, TextIndex(QUIET) );
	}

	attach_button_to_table( table, count++, Text(TOGGLE_FULLSCREEN), GTK_STOCK_FULLSCREEN, TextIndex(TOGGLE_FULLSCREEN) );
	if ( GetAVTransport() != NULL )
	{
		GtkWidget *button_clear = attach_button_to_table( table, count++, Text(CLEAR_PLAYLIST), GTK_STOCK_CLEAR, TextIndex(CLEAR_PLAYLIST) );
		gtk_widget_set_sensitive( button_clear, GetTrackCount() > 0 );

		if ( IsStopped() )
		{
			attach_button_to_table( table, ++count, Text(QUIT_APPLICATION), GTK_STOCK_QUIT, TextIndex(QUIT_APPLICATION) );
		}
		else
		{
			attach_button_to_table( table, count++, Text(QUIT_APPLICATION), GTK_STOCK_QUIT, TextIndex(QUIT_APPLICATION) );
			label = g_strconcat( Text(MEDIA_STOP), " & ", Text(MENU_QUIT), NULL );
			attach_button_to_table( table, count++, label, GTK_STOCK_QUIT, TextIndex(MENU_QUIT) );
			g_free( label );
		}
	}
	else
	{
		attach_button_to_table( table, count++, Text(QUIT_APPLICATION), GTK_STOCK_QUIT, TextIndex(QUIT_APPLICATION) );
	}

	gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(table) );

	gtk_widget_show_all( GTK_WIDGET(dialog) );
	response_id = gtk_dialog_run( dialog );
	gtk_widget_destroy( GTK_WIDGET(dialog) );

	switch ( response_id )
	{
	case TextIndex(BROWSER):
	case TextIndex(PLAYLIST):
	case TextIndex(NOW_PLAYING):
		SetCurrentPage( response_id - TextIndex(BROWSER) );
		break;

	case TextIndex(MEDIA_PLAY):
		DoPlay();
		break;
	case TextIndex(MEDIA_PAUSE):
		DoPause();
		break;
	case TextIndex(MEDIA_STOP):
		DoStop();
		break;
	case TextIndex(MEDIA_PREVIOUS):
		DoPreviousTrack();
		break;
	case TextIndex(MEDIA_NEXT):
		DoNextTrack();
		break;

	case TextIndex(TOGGLE_MUTE):
		ToggleMute();
		break;
	case TextIndex(QUIET):
	{
		GError *error = NULL;
		SetVolume( Settings->volume.quiet, &error );
		HandleError( error, Text(ERROR_VOLUME) );
	}
		break;

	case TextIndex(TOGGLE_FULLSCREEN):
		SetFullscreen( !GetFullscreen() );
		break;
	case TextIndex(CLEAR_PLAYLIST):
		DoPlaylistClear();
		break;

	case TextIndex(MENU_QUIT):
		if ( !DoStop() ) break;
		SetTransportState( TRANSPORT_STATE_STOPPED, NULL );  /* Stop Last.fm */
	case TextIndex(QUIT_APPLICATION):
		Quit();
		break;
	}
}

void keybd_select_renderer( int delta, gboolean long_press )
{
	const upnp_device *device = ChooseRenderer();
	if ( device != NULL ) SetRenderer( device, FALSE );
}

void keybd_refresh( int delta, gboolean long_press )
{
	extern void browser_refresh( void );
	extern void playlist_refresh( void );

	browser_refresh();
	playlist_refresh();
}

/*-----------------------------------------------------------------------------------------------*/

struct keybd_function keybd_functions[] =
{
	{ keybd_nop, ALLOW_REPEAT },
	{ keybd_fullscreen, SUPPRESS_REPEAT },
	{ keybd_page, SUPPORTS_LONG_PRESS },
	{ keybd_play_pause, SUPPORTS_LONG_PRESS },
	{ keybd_track, SUPPRESS_REPEAT },
	{ keydb_mute, SUPPRESS_REPEAT },
	{ keybd_volume, ALLOW_REPEAT },
	{ keybd_nop, ALLOW_REPEAT },
	{ keybd_selection, ALLOW_REPEAT },
	{ keybd_select_renderer, ALLOW_REPEAT },
	{ keybd_refresh, SUPPRESS_REPEAT }
};

struct keybd_function *get_keybd_function( GdkEventKey *event, gpointer user_data )
{
	int page_num = GPOINTER_TO_INT( user_data );
	struct keybd_function *func = NULL;

	TRACE(( "get_keybd_function( page_num = %d )\n", page_num ));

	switch ( event->keyval )
	{
	case GDK_F6:         /* Hardkey on Nokia IT devices */
		func = &keybd_functions[Settings->keys.full_screen];
		break;

	case GDK_Alt_L:      /* Hardkey on SmartQ devices */
		if ( IsSmartQ() ) func = &keybd_functions[Settings->keys.full_screen];
		break;

	case GDK_F7:         /* Hardkey on Nokia IT devices */
	case GDK_F8:         /* Hardkey on Nokia IT devices */
	case '+':
	case '-':
	case GDK_KP_Add:
	case GDK_KP_Subtract:
		func = &keybd_functions[Settings->keys.zoom];
		break;

	case GDK_Page_Up:    /* Hardkey on SmartQ devices */
	case GDK_Page_Down:  /* Hardkey on SmartQ devices */
		if ( IsSmartQ() ) func = &keybd_functions[Settings->keys.zoom];
		break;

#ifndef MAEMO

	case GDK_F11:
		func = &keybd_functions[KEYBD_FULLSCREEN];
		break;

	case 0xFFFFFF:
		switch ( event->hardware_keycode )
		{
		case 173:
			func = &keybd_functions[KEYBD_MUTE];
			break;
		case 174:
		case 175:
			func = &keybd_functions[KEYBD_VOLUME];
			break;
		case 176:
		case 177:
			func = &keybd_functions[KEYBD_TRACK];
			break;
		case 179:
			func = &keybd_functions[KEYBD_PLAY_PAUSE];
			break;
		}
		break;

#endif
	}

	if ( Settings->startup.classic_ui ) switch ( event->keyval )
	{
	case GDK_BackSpace:  /* Hardkey on Nokia IT devices */
		func = &keybd_functions[KEYBD_PAGE];
		break;

	case GDK_Escape:     /* Hardkey on Nokia IT & SmartQ devices */
		if ( Settings->MID ) func = &keybd_functions[KEYBD_PAGE];
		break;

	case GDK_ISO_Left_Tab:
	case GDK_Tab:
		func = &keybd_functions[KEYBD_TRACK];
		break;

	case GDK_Left:       /* Hardkey on Nokia IT devices */
	case GDK_Right:      /* Hardkey on Nokia IT devices */
		func = &keybd_functions[Settings->keys.left_right];
		break;

	case GDK_Up:         /* Hardkey on Nokia IT devices */
	case GDK_Down:       /* Hardkey on Nokia IT devices */
		func = &keybd_functions[Settings->keys.up_down];
		break;

	case GDK_space:
	case GDK_KP_Space:
	case GDK_Return:     /* Hardkey on Nokia IT devices */
	case GDK_KP_Enter:
		func = &keybd_functions[Settings->keys.enter];
		break;
	}
	else switch ( event->keyval )
	{
	case GDK_F5:
		func = &keybd_functions[KEYBD_REFRESH];
		break;
	}

	if ( page_num < 0 ) page_num = GetCurrentPage();

	switch ( page_num )
	{
	case BROWSER_PAGE:
		OnBrowserKey( event, &func );
		break;
	case PLAYLIST_PAGE:
		OnPlaylistKey( event, &func );
		break;
	case NOW_PLAYING_PAGE:
		OnNowPlayingKey( event, &func );
		break;
	}

	return func;
}

int get_keybd_delta( GdkEventKey *event )
{
	int delta = 1;

	switch ( event->keyval )
	{
	case GDK_Escape:
	case GDK_BackSpace:

	case GDK_Up:
	case GDK_Page_Up:
	case GDK_Left:

	case GDK_ISO_Left_Tab:
	case '-':
	case GDK_KP_Subtract:

	case GDK_F8:  /* '-' on Nokia IT */

		delta = -1;
		break;

	case 0xFFFFFF:
		switch ( event->hardware_keycode )
		{
		case 174:
		case 177:
			delta = -1;
			break;
		}
		break;
	}

	return delta;
}

/*-----------------------------------------------------------------------------------------------*/

static guint event_keyval;
static gboolean event_handled;

/* Callback for hardware keys */
gboolean KeyPressEvent( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	struct keybd_function *func;
	int repeat, delta;

#if LOGLEVEL
	if ( event->keyval == GDK_F12 )
	{
		Quit();
		return TRUE;
	}
#endif

	if ( Settings->debug.keybd )
	{
		gchar *text = g_strdup_printf( "keyval = %#x, keycode = %u", event->keyval, event->hardware_keycode );
		show_information( NULL, "Key Pressed: %s", text );
		g_free( text );
	}

	func = get_keybd_function( event, user_data );
	if ( func == NULL ) return FALSE;

	repeat = func->repeat;

#ifdef MAEMO
	if ( strcmp( osso_product_hardware, "SU-18" ) == 0 || strcmp( osso_product_hardware, "<unknown>" ) == 0 )
	{
		;  /* Nokia 770 */
	}
	else switch ( event->keyval )  /* Nokia N800/N810 */
	{
	case GDK_Return:
	case GDK_Escape:
	case GDK_F4:
	case GDK_F6:
	case GDK_F7:
	case GDK_F8:
		repeat = ALLOW_REPEAT;  /* The N800/810 suppresses repeat here anyway */
	}
#endif

	delta = get_keybd_delta( event );

	if ( repeat == ALLOW_REPEAT )
	{
		(*func->func)( delta, FALSE );
	}
	else if ( repeat == SUPPRESS_REPEAT )
	{
		if ( event_keyval != event->keyval )
		{
			(*func->func)( delta, FALSE );

			event_keyval = event->keyval;
			event_handled = TRUE;
		}
	}
	else if ( repeat == SUPPORTS_LONG_PRESS )
	{
		if ( event_keyval != event->keyval )
		{
			event_keyval = event->keyval;
			event_handled = FALSE;
		}
		else if ( !event_handled )
		{
			(*func->func)( delta, TRUE );
			event_handled = TRUE;
		}
	}

	return TRUE;
}

gboolean KeyReleaseEvent( GtkWidget *widget, GdkEventKey *event, gpointer user_data )
{
	struct keybd_function *func;

	if ( Settings->debug.keybd )
	{
		gchar *text = g_strdup_printf( "keyval = %#x, keycode = %u", event->keyval, event->hardware_keycode );
		show_information( NULL, "Key Released: %s", text );
		g_free( text );
	}

	func = get_keybd_function( event, user_data );
	if ( func == NULL ) return FALSE;

	if ( event_keyval == event->keyval )
	{
		if ( event_handled )
			event_handled = FALSE;
		else
			(*func->func)( get_keybd_delta( event ), FALSE );

		event_keyval = 0;
	}

	return TRUE;
}

/*-----------------------------------------------------------------------------------------------*/
