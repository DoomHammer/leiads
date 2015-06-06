/*
	miscellaneous.c
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
#include "md5.h"

/*-----------------------------------------------------------------------------------------------*/

const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

/*-----------------------------------------------------------------------------------------------*/

/* Note: Support for MD5 checksums has been added in GLib 2.16,
   but unfortunately we have only GLib 2.12 on the Nokia/OS2008 */
/* http://sourceforge.net/project/showfiles.php?group_id=42360 */

char *md5cpy( char *dest, const char *src )
{
	*dest = '\0';
	return md5cat( dest, src );
}

char *md5cat( char *dest, const char *src )
{
	md5_byte_t digest[16];
	md5_state_t state;
	char *result = dest;
	int i;

	md5_init( &state );
	md5_append( &state, (const md5_byte_t *)dest, strlen( dest ) );
	md5_append( &state, (const md5_byte_t *)src, strlen( src ) );
	md5_finish( &state, digest );

	for ( i = 0; i < 16; i++ )
	{
		unsigned char ch = digest[i];

		*dest++ = hex[ch >> 4];
		*dest++ = hex[ch & 0xF];
	}
	*dest++ = '\0';

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

gchar *get_common_str_part( gchar **titles, guint n )
{
	GString *str = g_string_new( "" );
	gchar *result, *end;
	guint i, j;

	for ( j = 0;; j++ )
	{
		gchar ch0 = titles[0][j];
		if ( ch0 == '\0' ) break;

		for ( i = 1; i < n; i++ )
		{
			gchar ch = titles[i][j];

			if ( ch == '\0' || ch != ch0 )
				break;
		}
		if ( i < n ) break;

		g_string_append_c( str, ch0 );
	}

	result = g_string_free( str, FALSE );
	if ( !g_utf8_validate( result, -1, (const gchar **)&end ) ) *end = '\0';

	return result;
}

gchar *make_valid_filename( gchar *filename )
{
	gchar *s = filename;

	for ( ;; s++ )
	{
		char ch = *s;

		if ( ch == '\0' )
			break;
		else if ( strchr( "\\/|", ch ) != NULL )
			*s = '-';
		else if ( strchr( ":*?\"<>", ch ) != NULL )
			*s = ' ';
	}

	return g_strstrip( filename );
}

/*-----------------------------------------------------------------------------------------------*/

int file_put_int( FILE *fp, const char *name, int value )
{
	ASSERT( name != NULL );
	return fprintf( fp, "<%s>%d</%s>", name, value, name );
}

int file_put_string( FILE *fp, const char *name, const char *value /*, const char *attrib_name_1, ...*/ )
{
	int result = 0;

	ASSERT( name != NULL );

	if ( value != NULL )
	{
		gchar *s;

	#if 0
		va_list args;
		va_start( args, attrib_name_1 );
		s = xml_box_v( name, value, attrib_name_1, args );
		va_end( args );
	#else
		s = xml_box( name, value, NULL );
	#endif

		result = fputs( s, fp );
		g_free( s );
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

int row_activated_flag;

static gboolean clear_row_activated_flag( gpointer data )
{
	row_activated_flag = FALSE;
	return FALSE;
}

void set_row_activated_flag( void )
{
	row_activated_flag = TRUE;
	g_idle_add( clear_row_activated_flag, NULL );
}

/*-----------------------------------------------------------------------------------------------*/

GtkIconSize get_icon_size_for_toolbar( GtkToolbar *toolbar )
{
	gboolean icon_size_set;
	GtkIconSize icon_size;

	g_object_get( toolbar, "icon-size", &icon_size, "icon-size-set", &icon_size_set, NULL );
	TRACE(( "get_icon_size_for_toolbar(): icon-size = %d, icon-size-set = %d\n", (int)icon_size, (int)icon_size_set ));
	if ( !icon_size_set )
	{
		GtkSettings *settings = get_settings_for_widget( GTK_WIDGET(toolbar) );

		g_object_get( settings, "gtk-toolbar-icon-size", &icon_size, NULL );
		TRACE(( "get_icon_size_for_toolbar(): gtk-toolbar-icon-size = %d\n", (int)icon_size ));
	}

	return icon_size;
}

gint get_icon_width_for_toolbar( GtkToolbar *toolbar )
{
	GtkSettings *settings = get_settings_for_widget( GTK_WIDGET(toolbar) );
	GtkIconSize icon_size = get_icon_size_for_toolbar( toolbar );
	gint width, height;

	if ( !gtk_icon_size_lookup_for_settings( settings, icon_size, &width, &height ) )
		width = height = (icon_size == GTK_ICON_SIZE_SMALL_TOOLBAR) ? 16 : 24;

	TRACE(( "get_icon_width_for_toolbar(): icon-width = %d, icon-height = %d\n", width, height ));
	return MAX( width, height );
}

#ifdef MAEMO
void fix_hildon_tool_button( GtkToolItem *item )
{
	if ( gtk_major_version == 2 && gtk_minor_version < 11 )
	{
		gtk_widget_set_size_request( GTK_WIDGET(item), 40, 40 );

		g_object_set( item, "can-default", FALSE, "can-focus", FALSE, "receives-default", FALSE, NULL );
		GTK_WIDGET_FLAGS( item ) &= ~(GTK_CAN_DEFAULT|GTK_CAN_FOCUS|GTK_RECEIVES_DEFAULT);
	}
}
#endif

GtkIconFactory *get_icon_factory( void )
{
	static GtkIconFactory *factory;

	if ( factory == NULL )
	{
		factory = gtk_icon_factory_new();
		ASSERT( factory != NULL );
		if ( factory != NULL )
		{
			gtk_icon_factory_add_default( factory );
			g_object_unref( factory );
		}
	}

	return factory;
}

const gchar *register_stock_icon( const gchar *stock_id, const guint8 *data )
{
	ASSERT( stock_id != NULL );

	if ( data != NULL )
	{
		GtkIconFactory *factory = get_icon_factory();

		GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline( -1, data, FALSE, NULL );
		ASSERT( pixbuf != NULL );
		if ( pixbuf != NULL )
		{
			GtkIconSet *icon_set = gtk_icon_set_new_from_pixbuf( pixbuf );
			gtk_icon_factory_add( factory, stock_id, icon_set );
			gtk_icon_set_unref( icon_set );

			g_object_unref( pixbuf );
			return stock_id;
		}
	}

	return NULL;
}

const gchar *register_bidi_stock_icon( const gchar *stock_id, const guint8 *data_ltr, const guint8 *data_rtl )
{
	ASSERT( stock_id != NULL );

	if ( data_ltr != NULL && data_rtl != NULL )
	{
		GtkIconFactory *factory = get_icon_factory();
		GtkIconSet *icon_set = gtk_icon_set_new();

		register_di_icon( icon_set, GTK_TEXT_DIR_LTR, data_ltr );
		register_di_icon( icon_set, GTK_TEXT_DIR_RTL, data_rtl );

		gtk_icon_factory_add( factory, stock_id, icon_set );
		gtk_icon_set_unref( icon_set );
		return stock_id;
	}

	return NULL;
}

/*
void register_sized_icon( GtkIconSet *icon_set, GtkIconSize size, const guint8 *data )
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline( -1, data, FALSE, NULL );
	ASSERT( pixbuf != NULL );
	if ( pixbuf != NULL )
	{
		GtkIconSource *source = gtk_icon_source_new();
		ASSERT( source != NULL );
		if ( source != NULL )
		{
			gtk_icon_source_set_size_wildcarded( source, FALSE );
			gtk_icon_source_set_size( source, size );
			gtk_icon_source_set_pixbuf( source, pixbuf );
			gtk_icon_set_add_source( icon_set, source );
			gtk_icon_source_free( source );
		}

		g_object_unref( pixbuf );
	}
}
*/

void register_di_icon( GtkIconSet *icon_set, GtkTextDirection direction, const guint8 *data )
{
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline( -1, data, FALSE, NULL );
	ASSERT( pixbuf != NULL );
	if ( pixbuf != NULL )
	{
		GtkIconSource *source = gtk_icon_source_new();
		ASSERT( source != NULL );
		if ( source != NULL )
		{
			gtk_icon_source_set_direction_wildcarded( source, FALSE );
			gtk_icon_source_set_direction( source, direction );
			gtk_icon_source_set_pixbuf( source, pixbuf );
			gtk_icon_set_add_source( icon_set, source );
			gtk_icon_source_free( source );
		}

		g_object_unref( pixbuf );
	}
}

void register_gtk_stock_icon( const gchar *stock_id )
{
	GtkIconFactory *factory = get_icon_factory();
	GtkIconSet *set = gtk_icon_set_new();
	GtkIconSource *source = gtk_icon_source_new();

	gtk_icon_source_set_icon_name( source, stock_id );
	gtk_icon_set_add_source( set, source );
	gtk_icon_source_free( source );

	gtk_icon_factory_add( factory, stock_id, set );
	gtk_icon_set_unref( set );
}

/*-----------------------------------------------------------------------------------------------*/

static gint busy_count;

void busy_enter( void /*GtkWidget* widget*/ )
{
	GtkWidget* widget = GTK_WIDGET(MainWindow);
	/*if ( widget == NULL ) widget = GTK_WIDGET(MainWindow);*/

	if ( busy_count++ == 0 )
	{
		GdkCursor* cursor = gdk_cursor_new/*_for_display*/(
			/*gtk_widget_get_display( widget, GDK_XTERM ),*/
			GDK_WATCH );
		gdk_window_set_cursor( gtk_widget_get_window( widget ), cursor );
		gdk_cursor_unref( cursor );
	}
}

void busy_leave( void /*GtkWidget* widget*/ )
{
	GtkWidget* widget = GTK_WIDGET(MainWindow);
	/*if ( widget == NULL ) widget = GTK_WIDGET(MainWindow);*/

	if ( --busy_count == 0 )
	{
		gdk_window_set_cursor( gtk_widget_get_window( widget ), NULL );
	}
	ASSERT( busy_count >= 0 );
}

/*-----------------------------------------------------------------------------------------------*/

void set_main_window_title( const char *title )
{
#ifdef MAEMO
	gtk_window_set_title( GTK_WINDOW(MainWindow), (title != NULL) ? title : "" );
#else
	if ( title != NULL && *title != '\0' )
	{
		gchar *window_title = g_strdup_printf( "%s - %s", NAME, title );
		gtk_window_set_title( MainWindow, window_title );
		g_free( window_title );
	}
	else
	{
		gtk_window_set_title( MainWindow, NAME );
	}
#endif
}

void set_screen_settings( void )
{
	Settings->screen_width  = gdk_screen_width();
	Settings->screen_height = gdk_screen_height();

#ifdef MAEMO
	Settings->MID = TRUE;
#else
{
	/*const gchar *desktop_session = g_getenv( "DESKTOP_SESSION" );*/  /* "LXDE" */
	Settings->MID = Settings->screen_width <= 800 && Settings->screen_height <= 480;  /* WVGA (800×480) */
}
#endif
}

/*-----------------------------------------------------------------------------------------------*/

gint message_dialog( GtkMessageType type, const gchar *description )
{
	GtkWindow* parent = GTK_WINDOW(MainWindow);
	gboolean cancel_as_default_button = FALSE;
#ifdef MAEMO
	const gchar *icon_name;
#endif
	GtkWidget* dialog;
	gint result;

	if ( type == GTK_MESSAGE_QUESTION_WITH_CANCEL_AS_DEFAULT_BUTTON )
	{
		type = GTK_MESSAGE_QUESTION;
		cancel_as_default_button = TRUE;
	}

#ifdef MAEMO

	switch ( type )
	{
	case GTK_MESSAGE_INFO:
		icon_name = GTK_STOCK_DIALOG_INFO;
		break;
	case GTK_MESSAGE_WARNING:
		icon_name = GTK_STOCK_DIALOG_WARNING;
		break;
	case GTK_MESSAGE_QUESTION:
		icon_name = GTK_STOCK_DIALOG_QUESTION;
		break;
	case GTK_MESSAGE_ERROR:
		icon_name = GTK_STOCK_DIALOG_ERROR;
		break;
	default:
		icon_name = NULL;
	}

	dialog = (( type == GTK_MESSAGE_QUESTION )
		? hildon_note_new_confirmation_with_icon_name
		: hildon_note_new_information_with_icon_name)
			( parent, description, icon_name );

#else

	dialog = gtk_message_dialog_new( parent, GTK_DIALOG_MODAL, type,
		( type == GTK_MESSAGE_QUESTION ) ? GTK_BUTTONS_OK_CANCEL : GTK_BUTTONS_OK,
		"%s", description );

#endif

	if ( cancel_as_default_button )
		gtk_dialog_set_default_response( GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL );

	gtk_widget_show_all( dialog );
	result = gtk_dialog_run( GTK_DIALOG(dialog) );
	gtk_widget_destroy( dialog );

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

#ifndef MAEMO
static gboolean animation_func( gpointer data )
{
	gboolean result = FALSE;

	gdk_threads_enter();

	if ( GTK_IS_WIDGET( data ) )
	{
		GtkProgressBar *progress_bar = GTK_PROGRESS_BAR( data );
		gtk_progress_bar_pulse( progress_bar );
		result = TRUE;
	}

	gdk_threads_leave();
	return result;
}
#endif

static GtkWidget *animation_show( const char *action, const gchar *item )
{
	GString *text = g_string_new( action );
	GtkWidget *animation;

#ifdef MAEMO

	if ( item != NULL )
	{
		g_string_append_c( text, ' ' );
		g_string_append_printf( text, Text(QUOTED), item );
	}

	animation = hildon_banner_show_animation( GTK_WIDGET(MainWindow), /*animation_name*/ NULL, text->str );

#else

	GtkWindow *parent = GTK_WINDOW(MainWindow);
	GtkVBox *vbox;
	GtkLabel *label;
	GtkProgressBar *progress_bar;
	GdkCursor* cursor;
	guint timeout_id;

	g_string_append( text, "..." );

	animation = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_type_hint( GTK_WINDOW(animation), GDK_WINDOW_TYPE_HINT_DIALOG );
	gtk_window_set_title( GTK_WINDOW(animation), ""/*text->str*/ );
	gtk_window_set_decorated( GTK_WINDOW(animation), TRUE );
	gtk_window_set_deletable( GTK_WINDOW(animation), FALSE );
	gtk_window_set_modal( GTK_WINDOW(animation), TRUE );
	gtk_window_set_transient_for( GTK_WINDOW(animation), parent );
	gtk_window_set_destroy_with_parent( GTK_WINDOW(animation), TRUE );
	gtk_container_set_border_width( GTK_CONTAINER(animation), 6 );

	vbox = GTK_VBOX( gtk_vbox_new( FALSE, 6 ) );
	gtk_container_add( GTK_CONTAINER(animation), GTK_WIDGET(vbox) );

	label = GTK_LABEL( new_label( text->str ) );
	gtk_box_pack_start_defaults( GTK_BOX(vbox), GTK_WIDGET(label) );

	progress_bar = GTK_PROGRESS_BAR( gtk_progress_bar_new() );
	if ( item != NULL ) gtk_progress_bar_set_text( progress_bar, item );
	/*gtk_widget_set_size_request( progress_bar, 150, -1 );*/
	gtk_box_pack_start_defaults( GTK_BOX(vbox), GTK_WIDGET(progress_bar) );

	gtk_window_set_position( GTK_WINDOW(animation), GTK_WIN_POS_CENTER_ON_PARENT );
	gtk_widget_show_all( animation );
	timeout_id = g_timeout_add( 250, animation_func, progress_bar );
	g_object_set_data( G_OBJECT(animation), "timeout_id", GUINT_TO_POINTER( timeout_id ) );

	cursor = gdk_cursor_new/*_for_display*/(
			/*gtk_widget_get_display( widget, GDK_XTERM ),*/
			GDK_WATCH );
	gdk_window_set_cursor( gtk_widget_get_window( animation ), cursor );
	gdk_cursor_unref( cursor );

#endif

	g_string_free( text, TRUE );
	return animation;
}

static gboolean animation_timeout( gpointer data )
{
	struct animation *a = (struct animation *)data;

	if ( a->timeout_id != 0 )  /* Thread is still running */
	{
		gdk_threads_enter();

		if ( a->timeout_id != 0 )  /* Thread is still running */
		{
			a->timeout_id = 0;

			a->widget = animation_show( a->action, a->item );
			g_free( a->item );
			a->item = NULL;
		}

		gdk_threads_leave();
	}

	return FALSE;
}

struct animation *show_animation( const char *action, const gchar *item, guint interval )
{
	struct animation *a = g_new0( struct animation, 1 );

	busy_enter();

	if ( interval != 0 )
	{
		a->action = /*g_strdup(*/ action /*)*/;
		a->item = g_strdup( item );
		a->timeout_id = g_timeout_add( interval, animation_timeout, a );
	}
	else
	{
		a->widget = animation_show( action, item );
	}

	return a;
}

void cancel_animation( struct animation *animation )
{
	if ( animation != NULL )
	{
		guint timeout_id = animation->timeout_id;
		animation->timeout_id = 0;
		if ( timeout_id != 0 ) g_source_remove( timeout_id );
	}
}

void clear_animation( struct animation *animation )
{
	if ( animation != NULL )
	{
		cancel_animation( animation );

		if ( animation->widget != NULL )
		{
		#ifndef MAEMO

			guint timeout_id = GPOINTER_TO_UINT( g_object_get_data( G_OBJECT(animation->widget), "timeout_id" ) );
			if ( timeout_id != 0 ) g_source_remove( timeout_id );

		#endif

			gtk_widget_destroy( animation->widget );
		}

		g_free( animation->item );
		/*g_free( animation->action );*/
		g_free( animation );

		busy_leave();
	}
}

/*-----------------------------------------------------------------------------------------------*/

#ifndef MAEMO

#define SHOW_INFORMATION_INTERVAL 2500

static struct show_information_data
{
	guint id, timeout_id;
	gboolean old_sensitive;
	gchar *old_label, *new_label;
} *sid;

static gboolean show_information_timeout( gpointer user_data )
{
	guint message_id = GPOINTER_TO_UINT( user_data );

	gdk_threads_enter();
	remove_from_statusbar( message_id );
	gdk_threads_leave();

	return FALSE;
}

#endif

void show_information( const gchar *icon_name, const gchar *text_format, const gchar *text )
{
#ifdef MAEMO

	hildon_banner_show_information( GTK_WIDGET(MainWindow), icon_name, text );

#else

	guint message_id = push_to_statusbar( text_format, text );
	guint timeout_id = g_timeout_add_full( G_PRIORITY_LOW, SHOW_INFORMATION_INTERVAL, show_information_timeout, GUINT_TO_POINTER( message_id ), NULL );
	if ( sid != NULL ) sid->timeout_id = timeout_id;

#endif
}

#ifndef MAEMO

guint push_to_statusbar( const gchar *text_format, const gchar *text )
{
	guint message_id = 0;

	if ( Statusbar != NULL )
	{
		gchar *statusbar_text;

		if ( text_format != NULL )
		{
			statusbar_text = g_strdup_printf( text_format, text );
		}
		else
		{
			statusbar_text = g_strdup( text );
		}

		ASSERT( statusbar_text != NULL );
		message_id = gtk_statusbar_push( Statusbar, StatusbarContext, statusbar_text );
		g_free( statusbar_text );
	}
	else if ( StatusMenuItem != NULL )
	{
		if ( sid != NULL )
		{
			gchar *label = NULL;

			g_source_remove( sid->timeout_id );

			g_object_get( StatusMenuItem, "label", &label, NULL );
			if ( label != NULL && sid->new_label != NULL && strcmp( sid->new_label, label ) == 0 )
			{
				;  /* menu_item_old_label is still valid */
			}
			else
			{
				/* invalidate menu_item_old_label */
				g_free( sid->old_label );
				sid->old_label = NULL;
			}
		}
		else
		{
			sid = g_new0( struct show_information_data, 1 );
		}

		if ( sid->old_label == NULL )
		{
			sid->old_sensitive = GTK_WIDGET_SENSITIVE( StatusMenuItem );
			g_object_get( StatusMenuItem, "label", &sid->old_label, NULL );
		}

		g_free( sid->new_label );
		sid->new_label = g_strdup( text );
		g_object_set( StatusMenuItem, "label", sid->new_label, NULL );
		gtk_widget_set_sensitive( GTK_WIDGET(StatusMenuItem), TRUE );

		message_id = ++(sid->id);
		if ( message_id == 0 ) message_id = ++(sid->id);
	}
	else
	{
		ASSERT( FALSE );
	}

	return message_id;
}

void remove_from_statusbar( guint message_id )
{
	if ( Statusbar != NULL )
	{
		gtk_statusbar_remove( Statusbar, StatusbarContext, message_id );
	}
	else if ( StatusMenuItem != NULL )
	{
		ASSERT( sid != NULL );
		if ( sid != NULL && sid->id == message_id )
		{
			gchar *label = NULL;
			g_object_get( StatusMenuItem, "label", &label, NULL );

			if ( label != NULL && strcmp( sid->new_label, label ) == 0 )
			{
				gtk_widget_set_sensitive( GTK_WIDGET(StatusMenuItem), sid->old_sensitive );
				g_object_set( StatusMenuItem, "label", sid->old_label, NULL );
			}

			g_free( label );

			g_free( sid->new_label );
			g_free( sid->old_label );
			g_free( sid );
			sid = NULL;
		}
	}
	else
	{
		ASSERT( FALSE );
	}
}

#endif

/*-----------------------------------------------------------------------------------------------*/

#ifdef MAEMO
#include <hildon/hildon-file-chooser-dialog.h>
#endif

void attach_to_table( GtkTable *table, GtkWidget *child, guint top_attach, guint left_attach, guint right_attach )
{
	gtk_table_attach( table, child, left_attach, right_attach, top_attach, top_attach + 1,
		GTK_IS_LABEL(child) ? GTK_FILL : GTK_FILL|GTK_EXPAND, GTK_FILL, 0, 0 );
}

gboolean get_alternative_button_order( void )
{
#ifdef MAEMO
	return TRUE;
#else
	GtkSettings *settings = gtk_settings_get_default();
	gboolean result = FALSE;
	g_object_get( settings, "gtk-alternative-button-order", &result, NULL );
	return result;
#endif
}

GList *get_selected_tree_row_references( GtkTreeSelection *selection )
{
	GList *rows, *rrs = NULL, *l;
	GtkTreeModel *model;

	rows = gtk_tree_selection_get_selected_rows( selection, &model );
	for ( l = rows; l != NULL; l = l->next )
	{
		GtkTreePath *path = (GtkTreePath *)l->data;
		rrs = g_list_append( rrs, gtk_tree_row_reference_new( model, path ) );
		gtk_tree_path_free( path );
	}
	g_list_free( rows );

	return rrs;
}

GtkSettings *get_settings_for_widget( GtkWidget *widget )
{
	GtkSettings *settings;

	if ( GTK_IS_WIDGET(widget) && gtk_widget_has_screen( widget ) )
		settings = gtk_settings_get_for_screen( gtk_widget_get_screen( widget ) );
	else
		settings = gtk_settings_get_default();

	return settings;
}

const GdkColor *get_widget_base_color( GtkWidget *widget, GtkStateType state )
{
	g_return_val_if_fail( GTK_IS_WIDGET(widget), NULL );
	return (widget->style != NULL) ? &widget->style->base[state] : NULL;
}

GdkWindow *get_widget_window( GtkWidget *widget )
{
	g_return_val_if_fail( GTK_IS_WIDGET(widget), NULL );
	return widget->window;
}

gboolean foreach_selected_tree_view_row( GtkTreeView *tree_view,
	gboolean (*func)( GtkTreeView *tree_view, GtkTreePath *path, gchar *path_str, gpointer user_data ), gpointer user_data )
{
	gboolean result = TRUE;

	GtkTreeSelection *selection = gtk_tree_view_get_selection( tree_view );
	GList *rows = gtk_tree_selection_get_selected_rows( selection, NULL );
	GList *row;

	for ( row = rows; row != NULL; row = row->next )
	{
		GtkTreePath *path = (GtkTreePath *)row->data;
		gchar *path_str = gtk_tree_path_to_string( path );

		if ( path_str != NULL )
		{
			result = (*func)( tree_view, path, path_str, user_data );
			g_free( path_str );

			if ( !result ) break;
		}
	}

	g_list_foreach( rows, (GFunc)gtk_tree_path_free, NULL );
	g_list_free( rows );

	return result;
}

GtkWidget *new_entry( gint max_length )
{
	GtkWidget *widget = gtk_entry_new();
	gtk_entry_set_max_length( GTK_ENTRY(widget), max_length );
	return widget;
}

GtkWidget *new_file_chooser_dialog( const gchar *title, GtkWindow *parent, GtkFileChooserAction action )
{
	GtkWidget *chooser;

#ifdef MAEMO

	if ( parent == NULL ) parent = GTK_WINDOW(MainWindow);
	chooser = hildon_file_chooser_dialog_new( parent, action );

#else

	const gchar *first_button_text, *second_button_text;
	GtkResponseType first_response, second_response;

	if ( parent == NULL ) parent = GTK_WINDOW(MainWindow);

	first_button_text = GTK_STOCK_CANCEL;
	first_response = GTK_RESPONSE_CANCEL;
	second_response = GTK_RESPONSE_OK;

	switch ( action )
	{
	case GTK_FILE_CHOOSER_ACTION_OPEN:
		second_button_text = GTK_STOCK_OPEN;
		break;

	case GTK_FILE_CHOOSER_ACTION_SAVE:
		second_button_text = GTK_STOCK_SAVE;
		break;

	default:
		ASSERT( FALSE );
		return NULL;
	}

	if ( get_alternative_button_order() )
		chooser = gtk_file_chooser_dialog_new( title, parent, action,
				second_button_text, second_response,
				first_button_text, first_response,
				NULL );
	else
		chooser = gtk_file_chooser_dialog_new( title, parent, action,
				first_button_text, first_response,
				second_button_text, second_response,
				NULL );

#endif

	return chooser;
}

GtkBox *new_hbox( void )
{
	return GTK_BOX(gtk_hbox_new( FALSE, SPACING ));
}

GtkWidget *new_label( const gchar *str )
{
	GtkWidget *label = gtk_label_new( str );
	gtk_misc_set_alignment( GTK_MISC(label), 0, 0.5 );
	return label;
}

GtkWidget *new_label_with_colon( const gchar *str )
{
	gchar *str_with_colon = g_strconcat( str, ":", NULL );
	GtkWidget *widget = new_label( str_with_colon );
	g_free( str_with_colon );
	return widget;
}

GtkDialog *new_modal_dialog_with_ok_cancel( const gchar *title, GtkWindow *parent )
{
	GtkDialog *dialog;

	if ( parent == NULL ) parent = GTK_WINDOW(MainWindow);

	dialog = GTK_DIALOG(( get_alternative_button_order() )
		? gtk_dialog_new_with_buttons( title, parent, GTK_DIALOG_MODAL,
			GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL )
		: gtk_dialog_new_with_buttons( title, parent, GTK_DIALOG_MODAL,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL ));
	gtk_dialog_set_default_response( dialog, GTK_RESPONSE_OK );

	return dialog;
}

GtkWidget *new_scrolled_window( void )
{
	GtkWidget *scrolled_window = gtk_scrolled_window_new( NULL, NULL );
	gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC );
	return scrolled_window;
}

GtkTable *new_table( guint rows, guint columns )
{
	GtkTable *table = GTK_TABLE(gtk_table_new( rows, columns, FALSE ));

	gtk_container_set_border_width( GTK_CONTAINER(table), BORDER );
	gtk_table_set_row_spacings( table, SPACING );
	gtk_table_set_col_spacings( table, SPACING );

	return table;
}

void set_cell_renderer_visible( GtkCellRenderer *cell, gboolean visible )
{
	g_return_if_fail( GTK_IS_CELL_RENDERER(cell) );
	g_object_set( cell, "visible", visible, NULL );
}

#define DIALOG_WIDTH_REQUEST      480
#define DIALOG_WIDTH_REQUEST_MID  640

void set_dialog_size_request( GtkDialog *dialog, gboolean set_height )
{
	gint width  = ( Settings->MID ) ? DIALOG_WIDTH_REQUEST_MID : DIALOG_WIDTH_REQUEST;
	gint height = ( set_height ) ? (width * 3) / 4 : -1;
	register gint tmp;

	tmp = (Settings->screen_width * 8) / 10;
	if ( width > tmp ) width = tmp;
	tmp = (Settings->screen_height * 8) / 10;
	if ( height > tmp ) height = tmp;

	gtk_widget_set_size_request( GTK_WIDGET(dialog), width, height );
}

#if GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION < 12
void set_tool_item_tooltip_text( GtkToolItem *tool_item, const gchar *text )
{
	static GtkTooltips *tooltips;

	if ( tooltips != NULL ) tooltips = gtk_tooltips_new();
	gtk_tool_item_set_tooltip( tool_item, tooltips, text, NULL );
}
#endif

void set_widget_visible( GtkWidget *widget, gboolean visible )
{
	g_return_if_fail( GTK_IS_WIDGET(widget) );
	g_object_set( widget, "visible", visible, NULL );
}

void tree_view_queue_resize( GtkTreeView *tree_view/*, int num_columns*/ )
{
	int i;

	for ( i = 0; /*i < num_columns*/; i++ )
	{
		GtkTreeViewColumn *column = gtk_tree_view_get_column( tree_view, i );
		if ( column == NULL ) break;
		gtk_tree_view_column_queue_resize( column );
	}
}

/*-----------------------------------------------------------------------------------------------*/

static void screenshot_show_info( const gchar *icon_name, const gchar *result )
{
	gdk_threads_enter();
	show_information( icon_name, "Screenshot: %s", result );
	gdk_threads_leave();
}

static void screenshot_save( GdkPixbuf *pb, const gchar *filename )
{
	if ( gdk_pixbuf_save( pb, filename, "png", NULL, NULL ) )
	{
		gchar *basename = g_path_get_basename( filename );
		gchar *result = g_strdup_printf( "Screenshot saved to %s.", basename );

		screenshot_show_info( GTK_STOCK_DIALOG_INFO, result );

		g_free( result );
		g_free( basename );
	}
	else
	{
		screenshot_show_info( GTK_STOCK_DIALOG_ERROR, "Unable to save the screenshot." );
	}
}

static gboolean screenshot_ask_filename( gpointer user_data )
{
	GdkPixbuf *pb = (GdkPixbuf *)user_data;

	GtkWidget *chooser;
	GtkFileFilter *filter;
	gchar *filename;

	gdk_threads_enter();

	chooser = new_file_chooser_dialog( "Save Screenshot", NULL, GTK_FILE_CHOOSER_ACTION_SAVE );

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name( filter, "Portable Network Graphics (*.png;*.PNG)" );
	gtk_file_filter_add_pattern( filter, "*.png;*.PNG" );
	gtk_file_chooser_add_filter( GTK_FILE_CHOOSER(chooser), filter );

	/*gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(chooser), default_folder_for_saving );*/
	gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER(chooser), "untitled.png" );
	gtk_file_chooser_set_filter( GTK_FILE_CHOOSER(chooser), filter );
	gtk_file_chooser_set_do_overwrite_confirmation( GTK_FILE_CHOOSER(chooser), TRUE );

	if ( gtk_dialog_run( GTK_DIALOG(chooser) ) == GTK_RESPONSE_OK )
		filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER(chooser) );
	else
		filename = NULL;

	gtk_widget_destroy( chooser );

	gdk_threads_leave();

	if ( filename != NULL )
	{
		screenshot_save( pb, filename );
		g_free( filename );
	}

	g_object_unref( pb );
	return FALSE;
}

static gboolean screenshot( gpointer user_data )
	/* taken from http://ubuntuforums.org/showthread.php?t=448160 */
	/* see also http://php-gtk.eu/code-hints/grabbing-a-screenshot-with-gdk */
{
	gchar *filename = (gchar*)user_data;

	GdkWindow *w = gdk_get_default_root_window();
	gint width, height;
	GdkPixbuf *pb;

	gdk_window_get_size( w, &width, &height );
	TRACE(( "The size of the window is %d x %d\n", width, height ));

	pb = gdk_pixbuf_get_from_drawable( NULL, w, gdk_window_get_colormap(w), 0, 0, 0, 0, width, height );
	if ( pb != NULL )
	{
		if ( filename == NULL )
		{
			g_idle_add( screenshot_ask_filename, pb );
		}
		else
		{
			screenshot_save( pb, filename );
			g_object_unref( pb );
		}
	}
	else
	{
		screenshot_show_info( GTK_STOCK_DIALOG_ERROR, "Unable to get the screenshot." );
	}

	g_free( filename );
	return FALSE;
}

void take_screenshot( guint delay, const char *filename )
{
	g_timeout_add( delay, screenshot, g_strdup( filename ) );
}

/*-----------------------------------------------------------------------------------------------*/
