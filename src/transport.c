/*
	transport.c
	Copyright (C) 2008-2010 Axel Sommerfeldt (axel.sommerfeldt@f-m.fm)

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

#define SHOW_TRANSPORT_STATE TRUE

#if SHOW_TRANSPORT_STATE
#ifdef MAEMO

#define LEIADS_STOCK_MEDIA_PLAY           GTK_STOCK_MEDIA_PLAY
#define LEIADS_STOCK_MEDIA_PAUSE          GTK_STOCK_MEDIA_PAUSE
#define LEIADS_STOCK_MEDIA_STOP           GTK_STOCK_MEDIA_STOP
#define LEIADS_STOCK_MEDIA_PREVIOUS       GTK_STOCK_MEDIA_PREVIOUS
#define LEIADS_STOCK_MEDIA_NEXT           GTK_STOCK_MEDIA_NEXT

#include "../icons/26x26/gtk+-2.10/leiads-media-transitioning-ltr.h"
#include "../icons/26x26/gtk+-2.10/leiads-media-transitioning-rtl.h"
#include "../icons/26x26/gtk+-2.10/leiads-media-playing-ltr.h"
#include "../icons/26x26/gtk+-2.10/leiads-media-playing-rtl.h"
#include "../icons/26x26/gtk+-2.10/leiads-media-paused.h"
#include "../icons/26x26/gtk+-2.10/leiads-media-stopped.h"

#else

#include "../icons/16x16/gtk+-2.12/gtk-media-play-ltr.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-play-ltr.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-play-rtl.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-play-rtl.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-pause.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-pause.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-stop.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-stop.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-previous-ltr.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-previous-ltr.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-previous-rtl.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-previous-rtl.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-next-ltr.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-next-ltr.h"
#include "../icons/16x16/gtk+-2.12/gtk-media-next-rtl.h"
#include "../icons/24x24/gtk+-2.12/gtk-media-next-rtl.h"

#define LEIADS_STOCK_MEDIA_PLAY           "leiads-media-play"
#define LEIADS_STOCK_MEDIA_PAUSE          "leiads-media-pause"
#define LEIADS_STOCK_MEDIA_STOP           "leiads-media-stop"
#define LEIADS_STOCK_MEDIA_PREVIOUS       "leiads-media-previous"
#define LEIADS_STOCK_MEDIA_NEXT           "leiads-media-next"

#include "../icons/16x16/gtk+-2.12/leiads-media-transitioning-ltr.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-transitioning-ltr.h"
#include "../icons/16x16/gtk+-2.12/leiads-media-transitioning-rtl.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-transitioning-rtl.h"
#include "../icons/16x16/gtk+-2.12/leiads-media-playing-ltr.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-playing-ltr.h"
#include "../icons/16x16/gtk+-2.12/leiads-media-playing-rtl.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-playing-rtl.h"
#include "../icons/16x16/gtk+-2.12/leiads-media-paused.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-paused.h"
#include "../icons/16x16/gtk+-2.12/leiads-media-stopped.h"
#include "../icons/24x24/gtk+-2.12/leiads-media-stopped.h"

#endif

#define LEIADS_STOCK_MEDIA_TRANSITIONING  "leiads-media-transitioning"
#define LEIADS_STOCK_MEDIA_PLAYING        "leiads-media-playing"
#define LEIADS_STOCK_MEDIA_PAUSED         "leiads-media-paused"
#define LEIADS_STOCK_MEDIA_STOPPED        "leiads-media-stopped"

#else

#define LEIADS_STOCK_MEDIA_PLAY           GTK_STOCK_MEDIA_PLAY
#define LEIADS_STOCK_MEDIA_PAUSE          GTK_STOCK_MEDIA_PAUSE
#define LEIADS_STOCK_MEDIA_STOP           GTK_STOCK_MEDIA_STOP
#define LEIADS_STOCK_MEDIA_PREVIOUS       GTK_STOCK_MEDIA_PREVIOUS
#define LEIADS_STOCK_MEDIA_NEXT           GTK_STOCK_MEDIA_NEXT

#endif

gboolean IsRadioTimeTrack, IsLastfmTrack;

GtkToolItem *ti_play, *ti_pause, *ti_stop, *ti_previous, *ti_next;

void set_tool_button_stock_id( GtkToolButton *button, const gchar *stock_id );

/*-----------------------------------------------------------------------------------------------*/

#if SHOW_TRANSPORT_STATE
static const struct media_icon_data
{
	const guint8 *play_ltr, *play_rtl;
	const guint8 *pause;
	const guint8 *stop;
	const guint8 *previous_ltr, *previous_rtl;
	const guint8 *next_ltr, *next_rtl;

	const guint8 *transitioning_ltr, *transitioning_rtl;
	const guint8 *playing_ltr, *playing_rtl;
	const guint8 *paused;
	const guint8 *stopped;
}
#ifdef MAEMO
icon_data[1] =
{
	{
		NULL, NULL,
		NULL,
		NULL,
		NULL, NULL,
		NULL, NULL,

		leiads_media_transitioning_ltr_10_26x26, leiads_media_transitioning_rtl_10_26x26,
		leiads_media_playing_ltr_10_26x26, leiads_media_playing_rtl_10_26x26,
		leiads_media_paused_10_26x26,
		leiads_media_stopped_10_26x26
	}
};
#else
icon_data[2] =
{
	{
		gtk_media_play_ltr_12_16x16, gtk_media_play_rtl_12_16x16,
		gtk_media_pause_12_16x16,
		gtk_media_stop_12_16x16,
		gtk_media_previous_ltr_12_16x16, gtk_media_previous_rtl_12_16x16,
		gtk_media_next_ltr_12_16x16, gtk_media_next_rtl_12_16x16,

		leiads_media_transitioning_ltr_12_16x16, leiads_media_transitioning_rtl_12_16x16,
		leiads_media_playing_ltr_12_16x16, leiads_media_playing_rtl_12_16x16,
		leiads_media_paused_12_16x16,
		leiads_media_stopped_12_16x16
	},
	{
		gtk_media_play_ltr_12_24x24, gtk_media_play_rtl_12_24x24,
		gtk_media_pause_12_24x24,
		gtk_media_stop_12_24x24,
		gtk_media_previous_ltr_12_24x24, gtk_media_previous_rtl_12_24x24,
		gtk_media_next_ltr_12_24x24, gtk_media_next_rtl_12_24x24,

		leiads_media_transitioning_ltr_12_24x24, leiads_media_transitioning_rtl_12_24x24,
		leiads_media_playing_ltr_12_24x24, leiads_media_playing_rtl_12_24x24,
		leiads_media_paused_12_24x24,
		leiads_media_stopped_12_24x24
	}
};
#endif
#endif

void CreateTransportToolItems( GtkToolbar *toolbar, gint icon_index )
{
#if SHOW_TRANSPORT_STATE
	const struct media_icon_data *data = icon_data + icon_index;

	register_bidi_stock_icon( LEIADS_STOCK_MEDIA_PLAY, data->play_ltr, data->play_rtl );
	register_stock_icon( LEIADS_STOCK_MEDIA_PAUSE, data->pause );
	register_stock_icon( LEIADS_STOCK_MEDIA_STOP, data->stop );
	register_bidi_stock_icon( LEIADS_STOCK_MEDIA_PREVIOUS, data->previous_ltr, data->previous_rtl );
	register_bidi_stock_icon( LEIADS_STOCK_MEDIA_NEXT, data->next_ltr, data->next_rtl );

	register_bidi_stock_icon( LEIADS_STOCK_MEDIA_TRANSITIONING, data->transitioning_ltr, data->transitioning_rtl );
	register_bidi_stock_icon( LEIADS_STOCK_MEDIA_PLAYING, data->playing_ltr, data->playing_rtl );
	register_stock_icon( LEIADS_STOCK_MEDIA_PAUSED, data->paused );
	register_stock_icon( LEIADS_STOCK_MEDIA_STOPPED, data->stopped );
#endif

	gtk_toolbar_insert( toolbar, gtk_separator_tool_item_new(), -1 );

	ti_play = gtk_tool_button_new_from_stock( LEIADS_STOCK_MEDIA_PLAY );
	fix_hildon_tool_button( ti_play );
	gtk_tool_item_set_tooltip_text( ti_play, Text(MEDIA_PLAY) );
	g_signal_connect( G_OBJECT(ti_play), "clicked", G_CALLBACK(DoPlay), NULL );
	gtk_toolbar_insert( toolbar, ti_play, -1 );

	ti_pause = gtk_tool_button_new_from_stock( LEIADS_STOCK_MEDIA_PAUSE );
	fix_hildon_tool_button( ti_pause );
	gtk_tool_item_set_tooltip_text( ti_pause, Text(MEDIA_PAUSE) );
	g_signal_connect( G_OBJECT(ti_pause), "clicked", G_CALLBACK(DoPause), NULL );
	gtk_toolbar_insert( toolbar, ti_pause, -1 );

	ti_stop = gtk_tool_button_new_from_stock( LEIADS_STOCK_MEDIA_STOP );
	fix_hildon_tool_button( ti_stop );
	gtk_tool_item_set_tooltip_text( ti_stop, Text(MEDIA_STOP) );
	g_signal_connect( G_OBJECT(ti_stop), "clicked", G_CALLBACK(DoStop), NULL );
	gtk_toolbar_insert( toolbar, ti_stop, -1 );

	ti_previous = gtk_tool_button_new_from_stock( LEIADS_STOCK_MEDIA_PREVIOUS );
	fix_hildon_tool_button( ti_previous );
	gtk_tool_item_set_tooltip_text( ti_previous, Text(MEDIA_PREVIOUS) );
	g_signal_connect( G_OBJECT(ti_previous), "clicked", G_CALLBACK(DoPreviousTrack), NULL );
	gtk_toolbar_insert( toolbar, ti_previous, -1 );

	ti_next = gtk_tool_button_new_from_stock( LEIADS_STOCK_MEDIA_NEXT );
	fix_hildon_tool_button( ti_next );
	gtk_tool_item_set_tooltip_text( ti_next, Text(MEDIA_NEXT) );
	g_signal_connect( G_OBJECT(ti_next), "clicked", G_CALLBACK(DoNextTrack), NULL );
	gtk_toolbar_insert( toolbar, ti_next, -1 );
}

void CreateTransportMenuItems( GtkMenuShell *menu )
{
	;  /* nothing to do */
}

int OpenTransport( void )
{
	TRACE(( "-> OpenTransport()\n" ));

	gtk_widget_set_sensitive( GTK_WIDGET(ti_play), TRUE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_pause), TRUE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_stop), TRUE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_previous), TRUE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_next), TRUE );

	TRACE(( "<- OpenTransport()\n" ));
	return 0;
}

void CloseTransport( void )
{
	TRACE(( "-> CloseTransport()\n" ));

	set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_play), LEIADS_STOCK_MEDIA_PLAY );
	set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_pause), LEIADS_STOCK_MEDIA_PAUSE );
	set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_stop), LEIADS_STOCK_MEDIA_STOP );

	gtk_widget_set_sensitive( GTK_WIDGET(ti_play), FALSE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_pause), FALSE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_stop), FALSE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_previous), FALSE );
	gtk_widget_set_sensitive( GTK_WIDGET(ti_next), FALSE );

	TRACE(( "<- CloseTransport()\n" ));
}

/*-----------------------------------------------------------------------------------------------*/

gboolean IsPlaying( void )
{
	gboolean result = FALSE;

	TRACE(( "-> IsPlaying()\n" ));

	result = (
		TransportState == TRANSPORT_STATE_TRANSITIONING ||
		TransportState == TRANSPORT_STATE_BUFFERING     ||
		TransportState == TRANSPORT_STATE_PLAYING );

	TRACE(( "<- IsPlaying( %d ) = %s\n", TransportState, (result) ? "TRUE" : "FALSE" ));
	return result;
}

gboolean IsStopped( void )
{
	gboolean result = FALSE;

	TRACE(( "-> IsStopped()\n" ));

	result = (
		TransportState == TRANSPORT_STATE_UNKNOWN ||
		TransportState == TRANSPORT_STATE_STOPPED ||
		TransportState == TRANSPORT_STATE_EOP     ||
		TransportState == TRANSPORT_STATE_NO_MEDIA_PRESENT );

	TRACE(( "<- IsStopped( %d ) = %s\n", TransportState, (result) ? "TRUE" : "FALSE" ));
	return result;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean DoPlay( void )
{
	GError *error = NULL;
	gboolean success;

	if ( IsLastfmTrack )
	{
		g_set_error( &error, 0, 0, Text(ERRMSG_LASTFM_RADIO) );
		success = FALSE;
	}
	else
	{
		busy_enter();
		success = Play( &error );
		busy_leave();
	}

	HandleError( error, Text(ERROR_PLAY) );
	return success;
}

gboolean DoPause( void )
{
	GError *error = NULL;
	gboolean success;

	if ( IsLastfmTrack ) return DoStop();

	busy_enter();
	success = Pause( &error );
	busy_leave();

	HandleError( error, Text(ERROR_PAUSE) );
	return success;
}

gboolean DoStop( void )
{
	GError *error = NULL;
	gboolean success;

	busy_enter();
	success = Stop( &error );
	busy_leave();

	HandleError( error, Text(ERROR_STOP) );
	return success;
}

gboolean DoPreviousTrack( void )
{
	GError *error = NULL;
	gboolean success;

	if ( IsLastfmTrack )
	{
		g_set_error( &error, 0, 0, Text(ERRMSG_LASTFM_RADIO) );
		success = FALSE;
	}
	else
	{
		busy_enter();
		success = SeekTracksRelative( -1, &error );
		busy_leave();
	}

	HandleError( error, Text(ERROR_SEEK_TRACK) );
	return success;
}

gboolean DoNextTrack( void )
{
	GError *error = NULL;
	gboolean success;

	busy_enter();
	success = SeekTracksRelative( +1, &error );
	busy_leave();

	HandleError( error, Text(ERROR_SEEK_TRACK) );
	return success;
}

/*-----------------------------------------------------------------------------------------------*/

void OnCurrentTrackIndex( int OldTrackIndex, int NewTrackIndex, gboolean Evented, gpointer user_data )
{
	gchar *lastfm_sk;

	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	TRACE(( "OnCurrentTrackIndex( %d => %d, %s )\n", OldTrackIndex, NewTrackIndex, (Evented) ? "TRUE" : "FALSE" ));

	IsRadioTimeTrack = IsRadioTimeItem( GetTrackItem( NewTrackIndex ) );
	TRACE(( "OnCurrentTrackIndex(): IsRadioTimeTrack = %s\n", (IsRadioTimeTrack) ? "TRUE" : "FALSE" ));

	lastfm_sk = GetLastfmSessionKeyFromTrack( NewTrackIndex );
	if ( lastfm_sk != NULL && !IsLastfmTrack )
	{
		IsLastfmTrack = TRUE;

		gtk_widget_set_sensitive( GTK_WIDGET(ti_pause), FALSE );
		gtk_widget_set_sensitive( GTK_WIDGET(ti_previous), FALSE );
	}
	else if ( lastfm_sk == NULL && IsLastfmTrack )
	{
		IsLastfmTrack = FALSE;

		gtk_widget_set_sensitive( GTK_WIDGET(ti_pause), TRUE );
		gtk_widget_set_sensitive( GTK_WIDGET(ti_previous), TRUE );
	}
	TRACE(( "OnCurrentTrackIndex(): IsLastfmTrack = %s\n", (IsLastfmTrack) ? "TRUE" : "FALSE" ));

	if ( Evented ) OnPlaylistTrackIndex( OldTrackIndex, NewTrackIndex );
	OnNowPlayingTrackIndex( OldTrackIndex, NewTrackIndex );

	if ( lastfm_sk != NULL )
	{
		if ( !IsStopped() && IsOurLastfmSession( lastfm_sk ) )
		{
			int track_count = GetTrackCount();
			ASSERT( NewTrackIndex >= 0 );
			if ( track_count > 0 && track_count - NewTrackIndex <= 2 )
			{
				GetLastfmRadioTracks();
			}
		}

		g_free( lastfm_sk );
	}

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

void OnTransportState( enum TransportState old_state, enum TransportState new_state, gpointer user_data )
{
	TRACE(( "OnTransportState( %d => %d )\n", old_state, new_state ));

	ASSERT( user_data == NULL || user_data == ENTER_GDK_THREADS );
	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_enter();

	OnPlaylistTransportState( new_state );

#if SHOW_TRANSPORT_STATE

	if ( new_state == TRANSPORT_STATE_TRANSITIONING || new_state == TRANSPORT_STATE_BUFFERING )
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_play), LEIADS_STOCK_MEDIA_TRANSITIONING );
	else if ( new_state == TRANSPORT_STATE_PLAYING)
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_play), LEIADS_STOCK_MEDIA_PLAYING );
	else
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_play), LEIADS_STOCK_MEDIA_PLAY );

	if ( new_state == TRANSPORT_STATE_PAUSED )
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_pause), LEIADS_STOCK_MEDIA_PAUSED );
	else
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_pause), LEIADS_STOCK_MEDIA_PAUSE );

	if ( new_state == TRANSPORT_STATE_STOPPED )
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_stop), LEIADS_STOCK_MEDIA_STOPPED );
	else
		set_tool_button_stock_id( GTK_TOOL_BUTTON(ti_stop), LEIADS_STOCK_MEDIA_STOP );

#endif

	if ( new_state == TRANSPORT_STATE_STOPPED )
	{
		gchar *lastfm_sk = GetLastfmSessionKeyFromTrack( CurrentTrackIndex );
		if ( lastfm_sk != NULL )
		{
			if ( IsOurLastfmSession( lastfm_sk ) )
			{
				http_server_remove_all_from_url_list();
				ClearPlaylist();
			}

			if ( strcmp( lastfm_sk, LastfmLeiaSessionKey ) != 0 )
				show_information( GTK_STOCK_MEDIA_STOP, NULL, Text(LASTFM_RADIO_STOPPED) );

			g_free( lastfm_sk );
		}
	}

	if ( user_data == ENTER_GDK_THREADS ) gdk_threads_leave();
}

/*-----------------------------------------------------------------------------------------------*/

void set_tool_button_stock_id( GtkToolButton *button, const gchar *stock_id )
{
	gchar *old_stock_id = NULL;
	g_object_get( button, "stock-id", &old_stock_id, NULL );

	ASSERT( old_stock_id != NULL );
	if ( old_stock_id != NULL )
	{
		if ( strcmp( old_stock_id, stock_id ) == 0 )
		{
			g_free( old_stock_id );
			return;
		}

		TRACE(( "set_tool_button_stock_id(): %s => %s\n", old_stock_id, stock_id ));
		g_free( old_stock_id );
	}

	gtk_tool_button_set_stock_id( button, stock_id );
}

/*-----------------------------------------------------------------------------------------------*/
