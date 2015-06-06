/*
	volume.c
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
#include <math.h>  /* floor() */

#define BUTTON_PRESS_TIMEOUT     100
#define MAX_VOLUME_DELTA         5
#define LIMIT_VOLUME_CHANGES     TRUE
#define LEVEL_CHANGED_TIMEOUT    2.5    /* in seconds */
#define SHOW_VOLUME_INFORMATION  FALSE
#define VOLUME_BAR_WIDTH         200

/*-----------------------------------------------------------------------------------------------*/

static GtkWidget *ti_separator;
static GtkWidget *ti_mute, *ti_minus, *ti_plus;
#ifdef GTK_TYPE_VOLUME_BUTTON
static GtkScaleButton *sb_level;
#endif
#ifdef MAEMO
static HildonVolumebar *hv_level;
#else
static GtkRange *r_level;
#endif
static GtkLabel *l_level;

static int volume_mute, volume_level;

static gulong id_mute_toggled, id_level_changed;
static GTimer *level_changed_timer;
static guint level_changed_timeout_id;

/*-----------------------------------------------------------------------------------------------*/

int  get_volume_limit( void );

void volumebar_show( void );
void volumebar_hide( void );
void volumebar_set_sensitive( gboolean sensitive );
void volumebar_set_mute_sensitive( gboolean sensitive );

void mute_toggled( void );
void button_clicked( GtkToolButton *toolbutton, gpointer user_data );
void level_changed( void );   /* (HildonVolumebar *hildonvolumebar, gpointer user_data) */
#ifdef GTK_TYPE_VOLUME_BUTTON
void scale_button_pressed( GtkButton *button, gpointer user_data );
#endif
#ifndef MAEMO
gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data );
#endif

/*-----------------------------------------------------------------------------------------------*/

static void insert_mute_button( GtkToolbar *toolbar )
{
	GtkToolItem *tool_item;

#ifdef STOCK_AUDIO_VOLUME_MUTE
	tool_item = gtk_toggle_tool_button_new_from_stock( STOCK_AUDIO_VOLUME_MUTE );
#else
	tool_item = gtk_toggle_tool_button_new_from_stock( STOCK_AUDIO_VOLUME_MUTED );
#endif
	gtk_tool_button_set_label( GTK_TOOL_BUTTON(tool_item), Text(MUTE) );
	gtk_tool_item_set_tooltip_text( tool_item, Text(MUTE) );
	id_mute_toggled = g_signal_connect( G_OBJECT(tool_item), "toggled", G_CALLBACK(mute_toggled), NULL );
	gtk_toolbar_insert( toolbar, tool_item, -1 );
	ti_mute = GTK_WIDGET(tool_item);
}

static void insert_toolbar_item( GtkToolbar *toolbar, GtkToolItem *item, GtkWidget *widget )
{
	g_object_set( widget, "can-focus", (gboolean)FALSE, NULL );

	gtk_container_add( GTK_CONTAINER(item), widget );
	gtk_tool_item_set_tooltip_text( item, Text(VOLUME) );
	gtk_toolbar_insert( toolbar, item, -1 );
}

static void insert_volume_label( GtkToolbar *toolbar, gfloat xalign )
{
	if ( Settings->volume.show_level )
	{
		GtkToolItem *tool_item;

		l_level = GTK_LABEL(gtk_label_new( "0" ));
		gtk_misc_set_alignment( GTK_MISC(l_level), xalign, 0.5 );

		tool_item = gtk_tool_item_new();
		gtk_tool_item_set_homogeneous( tool_item, TRUE );
		insert_toolbar_item( toolbar, tool_item, GTK_WIDGET(l_level) );
	}
}

void CreateVolumeToolItems( GtkToolbar *toolbar )
{
	GtkToolItem *tool_item;
	GtkBox *hbox;

	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_MUTED );
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_LOW );
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_MEDIUM );
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_HIGH );

#ifdef STOCK_AUDIO_VOLUME_VOLUME
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_VOLUME );
#endif
#ifdef STOCK_AUDIO_VOLUME_MUTE
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_MUTE );
#endif
#ifdef STOCK_AUDIO_VOLUME_MINUS
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_MINUS );
	register_gtk_stock_icon( STOCK_AUDIO_VOLUME_PLUS );
#endif

	if ( Settings->volume.ui != VOLUME_UI_NONE || Settings->volume.show_level )
	{
		tool_item = gtk_separator_tool_item_new();
		gtk_toolbar_insert( toolbar, tool_item, -1 );
		ti_separator = GTK_WIDGET(tool_item);
	}

	switch ( Settings->volume.ui )
	{
	case VOLUME_UI_NONE:
		if ( Settings->volume.show_level )
		{
		#ifdef STOCK_AUDIO_VOLUME_VOLUME
			GtkWidget *icon = gtk_image_new_from_stock( STOCK_AUDIO_VOLUME_VOLUME, get_icon_size_for_toolbar( toolbar ) );
		#else
			GtkWidget *icon = gtk_image_new_from_stock( STOCK_AUDIO_VOLUME_MEDIUM, get_icon_size_for_toolbar( toolbar ) );
		#endif
			tool_item = gtk_tool_item_new();
			gtk_tool_item_set_homogeneous( tool_item, TRUE );
			insert_toolbar_item( toolbar, tool_item, icon );

			insert_volume_label( toolbar, (gfloat)0.0 );
		}
		break;

	case VOLUME_UI_PLUSMINUS:
		insert_mute_button( toolbar );

	#ifdef STOCK_AUDIO_VOLUME_MINUS
		tool_item = gtk_tool_button_new_from_stock( STOCK_AUDIO_VOLUME_MINUS );
	#else
		tool_item = gtk_tool_button_new( gtk_label_new( "-" ), Text(VOLUME_MINUS) );
	#endif
		gtk_tool_item_set_tooltip_text( tool_item, Text(VOLUME_MINUS) );
		g_signal_connect( G_OBJECT(tool_item), "clicked", G_CALLBACK(button_clicked), GINT_TO_POINTER(-1) );
		gtk_toolbar_insert( toolbar, tool_item, -1 );
		ti_minus = GTK_WIDGET(tool_item);

		insert_volume_label( toolbar, (gfloat)0.5 );

	#ifdef STOCK_AUDIO_VOLUME_PLUS
		tool_item = gtk_tool_button_new_from_stock( STOCK_AUDIO_VOLUME_PLUS );
	#else
		tool_item = gtk_tool_button_new( gtk_label_new( "+" ), Text(VOLUME_PLUS) );
	#endif
		gtk_tool_item_set_tooltip_text( tool_item, Text(VOLUME_PLUS) );
		g_signal_connect( G_OBJECT(tool_item), "clicked", G_CALLBACK(button_clicked), GINT_TO_POINTER(+1) );
		gtk_toolbar_insert( toolbar, tool_item, -1 );
		ti_plus = GTK_WIDGET(tool_item);
		break;

#ifdef GTK_TYPE_VOLUME_BUTTON
	case VOLUME_UI_BUTTON:
		sb_level = GTK_SCALE_BUTTON( gtk_volume_button_new() );
	/*
		Workaround for bug in GTK+:
		Sometimes GTK+ emits a "value-changed" signal just by pressing the volume button.
		We try to prevent this.
	*/
		g_signal_connect( G_OBJECT(sb_level), "pressed", G_CALLBACK(scale_button_pressed), NULL );

		id_level_changed = g_signal_connect( G_OBJECT(sb_level), "value-changed", G_CALLBACK(level_changed), NULL );
		insert_toolbar_item( toolbar, gtk_tool_item_new(), GTK_WIDGET(sb_level) );

		insert_volume_label( toolbar, (gfloat)0.2 );
		break;
#endif

	case VOLUME_UI_BAR:
	#ifdef MAEMO

		hv_level = HILDON_VOLUMEBAR( hildon_hvolumebar_new() );
		id_mute_toggled = g_signal_connect( G_OBJECT(hv_level), "mute-toggled", G_CALLBACK(mute_toggled), NULL );
		id_level_changed = g_signal_connect( G_OBJECT(hv_level), "level-changed", G_CALLBACK(level_changed), NULL );

		tool_item = gtk_tool_item_new();
		gtk_tool_item_set_expand( tool_item, TRUE );
		insert_toolbar_item( toolbar, tool_item, GTK_WIDGET(hv_level) );

	#else

		insert_mute_button( toolbar );

		ti_minus = gtk_button_new_with_label( "-" );
		gtk_button_set_relief( GTK_BUTTON(ti_minus), GTK_RELIEF_NONE );
		g_signal_connect( ti_minus, "button-press-event", G_CALLBACK(button_press_event), GINT_TO_POINTER(-1) );
		g_signal_connect( ti_minus, "button-release-event", G_CALLBACK(button_release_event), NULL );

		ti_plus = gtk_button_new_with_label( "+" );
		gtk_button_set_relief( GTK_BUTTON(ti_plus), GTK_RELIEF_NONE );
		g_signal_connect( ti_plus, "button-press-event", G_CALLBACK(button_press_event), GINT_TO_POINTER(+1) );
		g_signal_connect( ti_plus, "button-release-event", G_CALLBACK(button_release_event), NULL );

		r_level = GTK_RANGE( gtk_hscale_new_with_range( 0, 100, 1 ) );
		gtk_scale_set_draw_value( GTK_SCALE( r_level ), FALSE );
		id_level_changed = g_signal_connect( G_OBJECT(r_level), "value-changed", G_CALLBACK(level_changed), NULL );

		hbox = GTK_BOX(gtk_hbox_new( FALSE, 0 ));
		gtk_box_pack_start( hbox, ti_minus, FALSE, FALSE, 0 );
		gtk_box_pack_end( hbox, ti_plus, FALSE, FALSE, 0 );
		gtk_container_add( GTK_CONTAINER(hbox), GTK_WIDGET(r_level) );

		tool_item = gtk_tool_item_new();
		if ( Settings->MID ) gtk_tool_item_set_expand( tool_item, TRUE );
		else gtk_widget_set_size_request( GTK_WIDGET(r_level), VOLUME_BAR_WIDTH, -1 );
		insert_toolbar_item( toolbar, tool_item, GTK_WIDGET(hbox) );

	#endif

		insert_volume_label( toolbar, (gfloat)0.5 );
		break;
	}
}

void CreateVolumeMenuItems( GtkMenuShell *menu )
{
	;  /* nothing to do */
}

int OpenVolume( void )
{
	if ( GetRenderingControl() != NULL )
	{
		volumebar_show();
		return TRUE;
	}
	else
	{
		volumebar_hide();
		return FALSE;
	}
}

void CloseVolume( void )
{
	if ( level_changed_timeout_id != 0 )
	{
		g_source_remove( level_changed_timeout_id );
		level_changed_timeout_id = 0;
	}
	if ( level_changed_timer != NULL )
	{
		g_timer_destroy( level_changed_timer );
		level_changed_timer = NULL;
	}

	volumebar_set_sensitive( FALSE );
}

/*-----------------------------------------------------------------------------------------------*/

void OnVolumeSettingsChanged( void )
{
	GtkAdjustment *adjustment = NULL;

	if ( ti_minus != NULL && (GTK_WIDGET_SENSITIVE(ti_minus) || GTK_WIDGET_SENSITIVE(ti_plus)) )
	{
		/* Update sensitivity of the "-" and "+" buttons */
		volumebar_set_mute_sensitive( TRUE );
	}

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
		adjustment = gtk_scale_button_get_adjustment( sb_level );
#endif
#ifdef MAEMO
	if ( hv_level != NULL )
		adjustment = hildon_volumebar_get_adjustment( hv_level );
#else
	if ( r_level != NULL )
		adjustment = gtk_range_get_adjustment( r_level );
#endif

	if ( adjustment != NULL )
	{
		int volume_limit = get_volume_limit();

		/* Relevant properties: "lower", "upper", and "page-increment" */
	#if LOGLEVEL
		gdouble lower = -1.0, upper = -1.0, page_increment = -1.0, page_size = -1.0, step_increment = -1.0;
		g_object_get( adjustment, "lower", &lower, "upper", &upper,
			"page-increment", &page_increment, "page-size", &page_size, "step-increment", &step_increment, NULL );
		TRACE(( "### lower = %f, upper = %f, page-increment = %f, page-size = %f, step-increment = %f\n",
			lower, upper, page_increment, page_size, step_increment ));
		/* => lower = 0.0, upper = 100.0, page-increment = 5.0, page-size = 0.0, step-increment = 5.0 */
	#endif

		TRACE(( "OnVolumeSettingsChanged(): volume_limit = %d\n", volume_limit ));
		g_object_set( adjustment, "lower", (gdouble)0.0, "upper", (gdouble)volume_limit,
			"page-increment", (gdouble)1.0, "page-size", (gdouble)0.0, "step-increment", (gdouble)1.0, NULL );

	#if LOGLEVEL
		g_object_get( adjustment, "lower", &lower, "upper", &upper,
			"page-increment", &page_increment, "page-size", &page_size, "step-increment", &step_increment, NULL );
		TRACE(( "### lower = %f, upper = %f, page-increment = %f, page-size = %f, step-increment = %f\n",
			lower, upper, page_increment, page_size, step_increment ));
	#endif
	}
}

/*-----------------------------------------------------------------------------------------------*/

void ToggleMute( void )
{
	GError *error = NULL;
	int mute;

	mute = (CurrentMute) ? FALSE : TRUE;

	SetMute( mute, &error );
	HandleError( error, Text(ERROR_TOGGLE_MUTE) );
}

void SetVolumeDelta( int delta )
{
	GError *error = NULL;
	int volume;

	if ( CurrentMute )
	{
		SetMute( FALSE, &error );
	}
	else
	{
		int volume_limit = get_volume_limit();

		volume = CurrentVolume + delta;
		if ( volume < 0 ) volume = 0;
		else if ( volume > volume_limit ) volume = volume_limit;

		SetVolume( volume, &error );
	}

	HandleError( error, Text(ERROR_VOLUME) );
}

/*-----------------------------------------------------------------------------------------------*/

int get_volume_limit( void )
{
	return ( Settings->volume.limit < VolumeLimit ) ? Settings->volume.limit : VolumeLimit;
}

/*-----------------------------------------------------------------------------------------------*/

void volumebar_show( void )
{
	volumebar_set_sensitive( TRUE );

	if ( ti_separator != NULL )
		gtk_widget_show( ti_separator );

	if ( ti_mute != NULL )
		gtk_widget_show( ti_mute );

	if ( ti_minus != NULL )
	{
		gtk_widget_show( ti_minus );
		gtk_widget_show( ti_plus );
	}

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
		gtk_widget_show( GTK_WIDGET(sb_level) );
#endif
#ifdef MAEMO
	if ( hv_level != NULL )
		gtk_widget_show( GTK_WIDGET(hv_level) );
#else
	if ( r_level != NULL )
		gtk_widget_show( GTK_WIDGET(r_level) );
#endif

	if ( l_level != NULL )
		gtk_widget_show( GTK_WIDGET(l_level) );
}

void volumebar_hide( void )
{
	if ( ti_separator != NULL )
		gtk_widget_hide( ti_separator );

	if ( ti_mute != NULL )
		gtk_widget_hide( ti_mute );

	if ( ti_minus != NULL )
	{
		gtk_widget_hide( ti_minus );
		gtk_widget_hide( ti_plus );
	}

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
		gtk_widget_hide( GTK_WIDGET(sb_level) );
#endif
#ifdef MAEMO
	if ( hv_level != NULL )
		gtk_widget_hide( GTK_WIDGET(hv_level) );
#else
	if ( r_level != NULL )
		gtk_widget_hide( GTK_WIDGET(r_level) );
#endif

	if ( l_level != NULL )
		gtk_widget_hide( GTK_WIDGET(l_level) );

	volumebar_set_sensitive( FALSE );
}

void volumebar_set_sensitive( gboolean sensitive )
{
	if ( ti_mute != NULL )
		gtk_widget_set_sensitive( ti_mute, sensitive );

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
		gtk_widget_set_sensitive( GTK_WIDGET(sb_level), sensitive );
#endif

#ifdef MAEMO
	if ( hv_level != NULL )
		gtk_widget_set_sensitive( GTK_WIDGET(hv_level), sensitive );
#endif

	volumebar_set_mute_sensitive( sensitive );
}

void volumebar_set_mute_sensitive( gboolean sensitive )
{
	if ( ti_minus != NULL )
	{
		int volume_limit = get_volume_limit();

		gtk_widget_set_sensitive( ti_minus, sensitive && (CurrentVolume > 0) );
		gtk_widget_set_sensitive( ti_plus, sensitive && (CurrentVolume < volume_limit) );
	}

#ifndef MAEMO
	if ( r_level != NULL )
		gtk_widget_set_sensitive( GTK_WIDGET(r_level), sensitive );
#endif

	if ( l_level != NULL )
		gtk_widget_set_sensitive( GTK_WIDGET(l_level), sensitive );
}

/*-----------------------------------------------------------------------------------------------*/

static int volumebar_get_mute( void )
{
	if ( ti_mute != NULL )
		return gtk_toggle_tool_button_get_active( GTK_TOGGLE_TOOL_BUTTON( ti_mute ) );

#ifdef MAEMO
	if ( hv_level != NULL )
		return hildon_volumebar_get_mute( hv_level );
#endif

	return volume_mute;
}

static void volumebar_set_mute( int mute )
{
	volume_mute = mute;

	if ( ti_mute != NULL )
	{
		g_signal_handler_block( G_OBJECT(ti_mute), id_mute_toggled );
		gtk_toggle_tool_button_set_active( GTK_TOGGLE_TOOL_BUTTON(ti_mute), mute != 0 );
		g_signal_handler_unblock( G_OBJECT(ti_mute), id_mute_toggled );
	}

#ifdef MAEMO
	if ( hv_level != NULL )
	{
		g_signal_handler_block( G_OBJECT(hv_level), id_mute_toggled );
		hildon_volumebar_set_mute( hv_level, mute );
		g_signal_handler_unblock( G_OBJECT(hv_level), id_mute_toggled );
	}
#endif
}

static int volumebar_get_level( void )
{
#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
	{
		gdouble level = gtk_scale_button_get_value( sb_level );
		return (int)floor( level + .5 );
	}
#endif

#if defined( MAEMO )
	if ( hv_level != NULL )
	{
		gdouble level = hildon_volumebar_get_level( hv_level );
		return (int)floor( level + .5 );
	}
#else
	if ( r_level != NULL )
	{
		gdouble level = gtk_range_get_value( r_level );
		return (int)floor( level + .5 );
	}
#endif

	return volume_level;
}

static void volumebar_set_label( int level )
{
	if ( ti_minus != NULL )
	{
		int volume_limit = get_volume_limit();

		gtk_widget_set_sensitive( ti_minus, level > 0 );
		gtk_widget_set_sensitive( ti_plus, level < volume_limit );
	}

	if ( l_level != NULL )
	{
		char str[4];
		sprintf( str, "%d", level );
		gtk_label_set_text( l_level, str );
	}

#if SHOW_VOLUME_INFORMATION
{
	gchar *text = g_strdup_printf( Text(VOLUME_INFO), volume_level );
	show_information( NULL, NULL, text );
	g_free( text );
}
#endif
}

static void volumebar_set_level( int level )
{
	volume_level = level;

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( sb_level != NULL )
	{
		g_signal_handler_block( G_OBJECT(sb_level), id_level_changed );
		gtk_scale_button_set_value( sb_level, (gdouble)level );
		g_signal_handler_unblock( G_OBJECT(sb_level), id_level_changed );
	}
#endif

#if defined( MAEMO )
	if ( hv_level != NULL )
	{
		g_signal_handler_block( G_OBJECT(hv_level), id_level_changed );
		hildon_volumebar_set_level( hv_level, (gdouble)level );
		g_signal_handler_unblock( G_OBJECT(hv_level), id_level_changed );
	}
#else
	if ( r_level != NULL )
	{
		g_signal_handler_block( G_OBJECT(r_level), id_level_changed );
		gtk_range_set_value( r_level, (gdouble)level );
		g_signal_handler_unblock( G_OBJECT(r_level), id_level_changed );
	}
#endif

	volumebar_set_label( level );
}

/*-----------------------------------------------------------------------------------------------*/

void mute_toggled( void )
{
	int mute = volumebar_get_mute();

	TRACE(( "# mute_toggled( %s )\n", (mute) ? "TRUE" : "FALSE" ));

	if ( mute != CurrentMute )
	{
		GError *error = NULL;
		gboolean success;

		busy_enter();

		success = SetMute( mute, &error );
		TRACE(( "SetMute( %d ) = %s\n", mute, (success) ? "TRUE" : "FALSE" ));
		if ( success )
		{
			CurrentMute = mute;
		}
		else
		{
			OnMute( CurrentMute, NULL );
		}

		HandleError( error, Text(ERROR_TOGGLE_MUTE) );

		busy_leave();
	}
}

void button_clicked( GtkToolButton *toolbutton, gpointer user_data )
{
	int delta = GPOINTER_TO_INT( user_data );

	TRACE(( "# button_clicked( %d )\n", delta ));

	SetVolumeDelta( delta );
}

static gboolean level_changed_timeout( gpointer data )
{
	gdouble elapsed = g_timer_elapsed( level_changed_timer, NULL );
	TRACE(( "level_changed_timeout(): elapsed = %f\n", elapsed ));
	if ( elapsed < LEVEL_CHANGED_TIMEOUT ) return TRUE;

	gdk_threads_enter();

	if ( level_changed_timeout_id != 0 )
	{
		GError *error = NULL;

		TRACE(( "* Enable preamp events...\n" ));

		level_changed_timeout_id = 0;
		EnableVolumeEvents( &error );
		HandleError( error, Text(ERROR_VOLUME) );
	}
	if ( level_changed_timer != NULL )
	{
		g_timer_destroy( level_changed_timer );
		level_changed_timer = NULL;
	}

	gdk_threads_leave();
	return FALSE;
}

#ifdef GTK_TYPE_VOLUME_BUTTON
static gboolean level_changed_locked;
#endif

void level_changed( void )
{
	int volume = volumebar_get_level();
	int delta = volume - CurrentVolume;

	TRACE(( "# level_changed( %d %+d => %d )\n", CurrentVolume, delta, volume ));

#ifdef GTK_TYPE_VOLUME_BUTTON
	if ( level_changed_locked )
	{
		TRACE(( "*** level_changed() locked!\n" ));
		if ( delta != 0 ) volumebar_set_level( CurrentVolume );
		return;
	}
#endif

#ifdef MAX_VOLUME_DELTA
	if ( delta < -MAX_VOLUME_DELTA || delta > MAX_VOLUME_DELTA )
	{
	#ifdef LIMIT_VOLUME_CHANGES
		if ( delta < -MAX_VOLUME_DELTA ) delta = -MAX_VOLUME_DELTA;
		else if ( delta > MAX_VOLUME_DELTA ) delta = MAX_VOLUME_DELTA;
	#else
		delta = 0;  /* deny change */
	#endif

		volume = CurrentVolume + delta;
		TRACE(( "## level_changed( %d %+d => %d )\n", CurrentVolume, delta, volume ));

		OnVolume( volume, NULL );  /* Make visual correction already here to avoid nasty effects, i.e. "jumping" volume slider */
	}
#endif

	if ( volume != CurrentVolume )
	{
		GError *error = NULL;
		gboolean success;

		busy_enter();

		if ( level_changed_timer == NULL )
			level_changed_timer = g_timer_new();
		else
			g_timer_reset( level_changed_timer );

		/* Disable volume events to make the volume bar work smooth */
		if ( level_changed_timeout_id == 0 )
		{
			TRACE(( "* Disable volume events...\n" ));

			DisableVolumeEvents( NULL );
			level_changed_timeout_id = g_timeout_add_full( G_PRIORITY_LOW, 1000, level_changed_timeout, NULL, NULL );
		}

		success = SetVolume( volume, &error );
		TRACE(( "SetVolume( %d ) = %s\n", volume, (success) ? "TRUE" : "FALSE" ));
		if ( success )
		{
			CurrentVolume = volume;
			volumebar_set_label( volume );
		}
		else
		{
			OnVolume( CurrentVolume, NULL );
		}

		HandleError( error, Text(ERROR_VOLUME) );

		busy_leave();
	}
}

#ifdef GTK_TYPE_VOLUME_BUTTON

static gboolean unlock_level_changed( gpointer data )
{
	level_changed_locked = FALSE;
	return FALSE;
}

void scale_button_pressed( GtkButton *button, gpointer user_data )
{
	GtkSettings *settings = get_settings_for_widget( GTK_WIDGET(button) );
	gint double_click_time = 250;  /* default value */

	TRACE(( "# scale_button_pressed()\n" ));

	level_changed_locked = TRUE;

	g_object_get( settings, "gtk-double-click-time", &double_click_time, NULL );
	TRACE(( "button_pressed(): gtk-double-click-time = %d\n", double_click_time ));

	g_timeout_add( double_click_time, unlock_level_changed, NULL );
}

#endif

#ifndef MAEMO

static guint button_press_timeout_id;

static gboolean _button_press_timeout( gpointer user_data )
{
	int level;

	if ( button_press_timeout_id == 0 ) return FALSE;

	level = volumebar_get_level() + GPOINTER_TO_INT( user_data );
	if ( level >= 0 && level <= get_volume_limit() )
	{
		gtk_range_set_value( r_level, (gdouble)level );
		return TRUE;
	}

	g_source_remove( button_press_timeout_id );
	button_press_timeout_id = 0;
	return FALSE;
}

static gboolean button_press_timeout( gpointer user_data )
{
	gboolean result;

	TRACE(( "# button_press_timeout( %d )\n", GPOINTER_TO_INT( user_data ) ));
	if ( button_press_timeout_id == 0 ) return FALSE;

	gdk_threads_enter();
	result = _button_press_timeout( user_data );
	gdk_threads_leave();
	return result;
}

gboolean button_press_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	TRACE(( "# button_press_event( %d )\n", GPOINTER_TO_INT( user_data ) ));

	if ( button_press_timeout_id == 0 )
		button_press_timeout_id = g_timeout_add( BUTTON_PRESS_TIMEOUT, button_press_timeout, user_data );

	_button_press_timeout( user_data );
	return TRUE;
}

gboolean button_release_event( GtkWidget *widget, GdkEventButton *event, gpointer user_data )
{
	TRACE(( "# button_release_event()\n" ));

	if ( button_press_timeout_id != 0 )
	{
		g_source_remove( button_press_timeout_id );
		button_press_timeout_id = 0;
	}

	return TRUE;
}

#endif

/*-----------------------------------------------------------------------------------------------*/

void OnVolumeInit( gpointer user_data )
{
	TRACE(( "OnVolumeInit(): VolumeLimit = %d\n", VolumeLimit ));
	ASSERT( user_data == NULL );

	OnVolumeSettingsChanged();
}

void OnMute( gboolean mute, gpointer user_data )
{
	TRACE(( "OnMute( %d )\n", mute ));
	ASSERT( user_data == NULL );

	if ( mute != volumebar_get_mute() )
	{
		volumebar_set_mute( mute );
	}

	volumebar_set_mute_sensitive( !mute );
}

void OnVolume( int volume, gpointer user_data )
{
	TRACE(( "OnVolume( %d )\n", volume ));
	ASSERT( user_data == NULL );

	if ( volume != volumebar_get_level() )
	{
		volumebar_set_level( volume );
	}
}

/*-----------------------------------------------------------------------------------------------*/
