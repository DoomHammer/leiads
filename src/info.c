/*
	info.c
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
#include <ctype.h>

/*-----------------------------------------------------------------------------------------------*/

#define INFO_BUSY_TIMEOUT      1000  /* Show busy animation after 1s */
#define MAX_INFO_LEN             40

struct info_data
{
	upnp_device *device;
	struct animation *animation;
};

struct linn_data
{
	gchar *software_version, *hardware_version;
	gchar *mac_address, *product_id;
	long non_seekable_buffer;
};

void append_info( GtkListStore *store, const char *name, const char *value )
{
	AppendInfo( store, name, ( value != NULL ) ? value : "?", 0 );
}

GtkWidget *create_info( const upnp_device *device )
{
	const char *ds_compatibility_family = upnp_get_ds_compatibility_family( device );

	GtkTreeView *view;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkListStore *store;

	store = gtk_list_store_new( 2, G_TYPE_STRING, G_TYPE_STRING );

	AppendInfo( store, Text(INFO_MODEL_NAME), device->modelName, MAX_INFO_LEN );
	AppendInfo( store, Text(INFO_MODEL_NUMBER), device->modelNumber, MAX_INFO_LEN );

	if ( ds_compatibility_family != NULL )
	{
		struct linn_data *data = (struct linn_data *)device->user_data;
		gchar *value;

		ASSERT( data != NULL );

		value = upnp_get_ip_address( device );
		append_info( store, Text(INFO_IP_ADDRESS), value );
		g_free( value );

		if ( data->mac_address != NULL )
		{
			GString *mac_address = g_string_new( data->mac_address );

			/* Auskerry and Bute report "123456789012" while Cara reports "12:34:56:78:90:12" */
			if ( strchr( mac_address->str, ':' ) == NULL )
			{
				gssize pos;
				for ( pos = 2; pos <= 14; pos += 3 )
					g_string_insert_c( mac_address, pos, ':' );
			}

			append_info( store, Text(INFO_MAC_ADDRESS), mac_address->str );
			g_string_free( mac_address, TRUE );
		}
		else
		{
			append_info( store, Text(INFO_MAC_ADDRESS), NULL );
		}

		append_info( store, Text(INFO_PRODUCT_ID), data->product_id );

		value = g_strdup_printf( "%s (%s)", ( data->software_version != NULL ) ? data->software_version : "?", ds_compatibility_family );
		append_info( store, Text(INFO_SOFTWARE_VERSION), value );
		g_free( value );

		if ( data->non_seekable_buffer >= 0 )
		{
			char value[16];
			sprintf( value, "%ld", data->non_seekable_buffer );
			append_info( store, "Buffer padding", value );
		}
	}
	else if ( upnp_device_is_external_preamp( device ) )
	{
		struct linn_data *data = (struct linn_data *)device->user_data;

		append_info( store, Text(INFO_SOFTWARE_VERSION), data->software_version );
		append_info( store, Text(INFO_HARDWARE_VERSION), data->hardware_version );
	}
	else
	{
		gchar *value;

		AppendInfo( store, Text(INFO_MANUFACTURER), device->manufacturer, MAX_INFO_LEN );
		AppendInfo( store, Text(INFO_URL), device->manufacturerURL, MAX_INFO_LEN );

		value = upnp_get_ip_address( device );
		append_info( store, Text(INFO_IP_ADDRESS), value );
		g_free( value );
	}

#if LOGLEVEL > 1
	AppendInfo( store, "URLBase", device->URLBase, 0 );
	AppendInfo( store, "serialNumber", device->serialNumber, 0 );
#endif

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
	return GTK_WIDGET(view);
}

gboolean info_thread_end( gpointer user_data )
{
	struct info_data *data = (struct info_data *)user_data;

	GtkDialog *dialog;

	gdk_threads_enter();

	clear_animation( data->animation );

	dialog = GTK_DIALOG(gtk_dialog_new_with_buttons(
		( data->device != NULL ) ? data->device->friendlyName : Text(INFO),
		GTK_WINDOW(MainWindow), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL ));
	gtk_dialog_set_has_separator( dialog, FALSE );
	gtk_dialog_set_default_response( dialog, GTK_RESPONSE_OK );

	if ( data->device == NULL )
	{
		struct device_list_entry *sr_list;
		GtkNotebook *notebook;
		int i, n;

		set_dialog_size_request( dialog, FALSE );

		/* Create notebook */
		notebook = GTK_NOTEBOOK(gtk_notebook_new());
		/*gtk_notebook_set_tab_pos( notebook, GTK_POS_UP );*/
		gtk_notebook_set_scrollable( notebook, TRUE );

		sr_list = GetRendererList( &n );
		ASSERT( sr_list != NULL );

		for ( i = 0; i < n; i++ )
		{
			const upnp_device *device = sr_list[i].device;
			ASSERT( device != NULL );
			if ( device == NULL ) break;

			TRACE(( "Renderer = \"%s\"\n", device->friendlyName ));
			gtk_notebook_append_page( notebook, create_info( device ),
				GTK_WIDGET(gtk_label_new( device->friendlyName )) );

			device = upnp_get_corresponding_rendering_control( device );
			if ( upnp_device_is_external_preamp( device ) )
			{
				TRACE(( "Preamp = \"%s\"\n", device->friendlyName ));
				gtk_notebook_append_page( notebook, create_info( device ),
					GTK_WIDGET(gtk_label_new( device->friendlyName )) );
			}
		}

		g_free( sr_list );

		sr_list = GetServerList( &n );
		ASSERT( sr_list != NULL );

		for ( i = 0; i < n; i++ )
		{
			const upnp_device *device = sr_list[i].device;
			ASSERT( device != NULL );
			if ( device == NULL ) break;

			TRACE(( "Server = \"%s\"\n", device->friendlyName ));
			gtk_notebook_append_page( notebook, create_info( device ),
				GTK_WIDGET(gtk_label_new( device->friendlyName )) );
		}

		g_free( sr_list );

		gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), GTK_WIDGET(notebook) );
	}
	else
	{
		TRACE(( "Device = \"%s\"\n", data->device->friendlyName ));
		gtk_box_pack_start_defaults( GTK_BOX(dialog->vbox), create_info( data->device ) );
	}

	gtk_widget_show_all( GTK_WIDGET(dialog) );
	gtk_dialog_run( dialog );
	gtk_widget_destroy( GTK_WIDGET(dialog) );

	gdk_threads_leave();

	g_free( data );
	return FALSE;
}

static void free_linn_data( void *user_data )
{
	struct linn_data *data = (struct linn_data *)user_data;

	if ( data != NULL )
	{
		g_free( data->product_id );
		g_free( data->mac_address );
		g_free( data->hardware_version );
		g_free( data->software_version );

		g_free( data );
	}
}

static struct linn_data *get_linn_data( upnp_device *device )
{
	struct linn_data *data = (struct linn_data *)device->user_data;

	if ( data == NULL )
	{
		data = g_new0( struct linn_data, 1 );
		data->non_seekable_buffer = -1;

		device->user_data = data;
		device->free_user_data = free_linn_data;
	}

	return data;
}

static void get_info( upnp_device *device )
{
	const char *ds_compatibility_family = upnp_get_ds_compatibility_family( device );
	GError *error = NULL;

	if ( ds_compatibility_family != NULL )
	{
		struct linn_data *data = get_linn_data( device );
		upnp_string value;

		if ( error == NULL && data->software_version == NULL )
		{
			if ( Linn_Volkano_SoftwareVersion( device, &value, &error ) )
			{
				data->software_version = g_strdup( value );
				upnp_free_string( value );
			}
		}

		if ( error == NULL && data->mac_address == NULL )
		{
			if ( Linn_Volkano_MacAddress( device, &value, &error ) )
			{
				data->mac_address = g_strdup( value );
				upnp_free_string( value );
			}
		}

		if ( error == NULL && data->product_id == NULL )
		{
			if ( Linn_Volkano_ProductId( device, &value, &error ) )
			{
				data->product_id = g_strdup( value );
				upnp_free_string( value );
			}
		}

		if ( error == NULL && data->non_seekable_buffer == -1 && strcmp( ds_compatibility_family, "Bute" ) == 0 )
		{
			upnp_ui4 value;

			if ( Linn_Media_NonSeekableBuffer( device, &value, &error ) )
				data->non_seekable_buffer = (long)value;
			else
				data->non_seekable_buffer = -2;
		}
	}
	else if ( upnp_device_is_external_preamp( device ) )
	{
		struct linn_data *data = get_linn_data( device );
		upnp_string value;

		if ( error == NULL && data->software_version == NULL )
		{
			if ( Preamp_SoftwareVersion( device, &value, &error ) )
			{
				data->software_version = g_strdup( value );
				upnp_free_string( value );
			}
		}

		if ( error == NULL && data->hardware_version == NULL )
		{
			if ( Preamp_HardwareVersion( device, &value, &error ) )
			{
				data->hardware_version = g_strdup( value );
				upnp_free_string( value );
			}
		}
	}

	HandleError( error, Text(ERROR_DEVICE_INFO_FROM), device->friendlyName );
}

gpointer info_thread( gpointer user_data )
{
	struct info_data *data = (struct info_data *)user_data;

	if ( data->device == NULL )
	{
		upnp_device *device;

		for ( device = upnp_get_first_device( NULL ); device != NULL; device = upnp_get_next_device( device, NULL ) )
		{
			get_info( device );
		}
	}
	else
	{
		get_info( data->device );
	}

	cancel_animation( data->animation );
	g_idle_add( info_thread_end, data );
	return NULL;
}

void Info( upnp_device *device )
{
	struct info_data *data = g_new0( struct info_data, 1 );
	GError *error = NULL;

	data->device = device;
	data->animation = show_animation( Text(MENU_DEVICE_INFO), NULL, INFO_BUSY_TIMEOUT );
	if ( g_thread_create( info_thread, data, FALSE, &error ) == NULL )
	{
		HandleError( error, Text(ERROR_DEVICE_INFO) );

		clear_animation( data->animation );
		g_free( data );
	}
}

/*-----------------------------------------------------------------------------------------------*/

gboolean IsValuableInfo( const char *value )
{
	return ( value != NULL && *value != '\0' &&
	/*	strcmp( value, "Unknown" ) != 0 && strncmp( value, "(Unknown ", 9 ) != 0 && */
		strcmp( value, "0000-01-01" ) != 0 );
}

gchar *GetInfo( const gchar *item_or_container, const char *name, char *value, size_t sizeof_value )
{
	if ( value != NULL )
	{
		value = xml_get_named( item_or_container, name, value, sizeof_value );
		if ( !IsValuableInfo( value ) )
		{
			value = NULL;
		}
	}
	else
	{
		value = xml_get_named_str( item_or_container, name );
		if ( !IsValuableInfo( value ) )
		{
			g_free( value );
			value = NULL;
		}
	}

	return value;
}

gboolean AppendInfo( GtkListStore *store, const char *name, const char *value, int maxlen )
{
	ASSERT( store != NULL && name != NULL );

	if ( IsValuableInfo( value ) )
	{
		GString *name_with_colon;
		gboolean multiline;
		gchar *buf = NULL;
		GtkTreeIter iter;

		if ( maxlen < 0 )
		{
			multiline = TRUE;
			maxlen = -maxlen;
		}
		else
		{
			multiline = FALSE;
		}

		do
		{
			if ( maxlen > 0 && g_utf8_strlen( value, -1 ) > maxlen )
			{
				const gchar *s = value, *space = NULL;
				int n = maxlen;

				if ( buf == NULL ) buf = g_new( gchar, strlen( value ) + 4 );
				/*if ( !multiline ) n -= 2;*/

				while ( n )
				{
					s = g_utf8_next_char( s );
					if ( *s == ' ' ) space = s;
					n--;
				}

				if ( multiline && space != NULL ) s = space;
				strncpy( buf, value, s - value )[s - value] = '\0';

				if ( multiline )
				{
					value = s;
					while ( *value == ' ' ) value++;
				}
				else
				{
					strcat( buf, "..." );
					maxlen = 0;
				}
			}
			else if ( buf != NULL )
			{
				g_free( buf );
				buf = NULL;

				if ( !multiline ) break;
			}

			gtk_list_store_append( store, &iter );

			if ( name != NULL )
			{
				name_with_colon = g_string_new( name );
				g_string_append_c( name_with_colon, ':' );
				gtk_list_store_set( store, &iter, 0, name_with_colon->str, 1, (buf != NULL) ? buf : value, -1 );
				g_string_free( name_with_colon, TRUE );

				name = NULL;
			}
			else
			{
				gtk_list_store_set( store, &iter, 1, (buf != NULL) ? buf : value, -1 );
			}
		}
		while ( buf != NULL );

		return TRUE;
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

static gchar *de_standardize( gchar *value )
{
	if ( value != NULL )
	{
		size_t len = strlen( value );
		gchar *s, *the = NULL;

		if ( len > 5 )
		{
			s = value + len - 5;

			if ( strcmp( s, ", The" ) == 0 || strcmp( s, ", Die" ) == 0 )
			{
				the = s + 2;
			}
		}
		if ( the == NULL && len > 6 )
		{
			s = value + len - 6;

			if ( strcmp( s, " (The)" ) == 0 || strcmp( s, " (Die)" ) == 0 )
			{
				the = s + 2;
			}
		}

		if ( the != NULL )
		{
			char buf[4];

			buf[0] = the[0];
			buf[1] = the[1];
			buf[2] = the[2];

			*s++ = '\0';
			memmove( value + 4, value, s - value );

			value[0] = buf[0];
			value[1] = buf[1];
			value[2] = buf[2];
			value[3] = ' ';
		}
	}

	return value;
}


gchar *GetAlbum( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	gchar *album = GetInfo( item_or_container, "upnp:album", value, sizeof_value );
	if ( album == NULL )
	{
		char buf[34];

		if ( GetInfo( item_or_container, "upnp:class", buf, sizeof( buf ) ) != NULL )
		{
			if ( strcmp( buf, "object.container.album.musicAlbum" ) == 0 )
			{
				album = GetInfo( item_or_container, "dc:title", value, sizeof_value );
				if ( album != NULL )
				{
					gchar *s;

					/* Remove annotations */
					s = strstr( album, " (" );
					if ( s != NULL ) *s = '\0';
					s = strstr( album, " [" );
					if ( s != NULL ) *s = '\0';
					s = strstr( album, " {" );
					if ( s != NULL ) *s = '\0';
				}
			}
		}
	}

	return de_standardize( album );
}

gchar *GetAlbumArtist( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	const gchar *xml = item_or_container;
	gchar *artist = NULL;

	for (;;)
	{
		struct xml_info info;
		char role[16];

		xml = xml_find_named( xml, "upnp:artist", &info );
		if ( xml == NULL ) break;

		if ( xml_get_named_attribute( xml, "role", role, sizeof( role ) ) != NULL && strcmp( role, "AlbumArtist" ) == 0 )
		{
			artist = ( value != NULL )
				? xml_strcpy( value, sizeof_value, info.content, info.end_of_content )
				: xml_gstrcpy( info.content, info.end_of_content );
			break;
		}

		xml = info.next;
	}

	return de_standardize( artist );
}

gchar *GetArtist( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	const gchar *xml = item_or_container;
	gchar *artist = NULL;

	for (;;)
	{
		struct xml_info info;
		char role[16];

		xml = xml_find_named( xml, "upnp:artist", &info );
		if ( xml == NULL ) break;

		if ( xml_get_named_attribute( xml, "role", role, sizeof( role ) ) != NULL && strcmp( role, "Performer" ) == 0 )
		{
			artist = ( value != NULL )
				? xml_strcpy( value, sizeof_value, info.content, info.end_of_content )
				: xml_gstrcpy( info.content, info.end_of_content );
			break;
		}

		xml = info.next;
	}

	if ( artist == NULL ) artist = GetInfo( item_or_container, "upnp:artist", value, sizeof_value );
	if ( artist == NULL ) artist = GetInfo( item_or_container, "dc:creator", value, sizeof_value );
	return de_standardize( artist );
}

gchar *GetDate( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	char date[32];

	if ( xml_get_named( item_or_container, "dc:date", date, sizeof( date ) ) != NULL &&
	     IsValuableInfo( date ) )
	{
		if ( strlen( date ) == 10 && strcmp( date + 4, "-01-01" ) == 0 )
		{
			date[4] = '\0';
		}

		return ( value != NULL ) ? (( sizeof_value > strlen( date ) ) ? strcpy( value, date ) : NULL) : g_strdup( date );
	}

	return NULL;
}

gchar *GetDuration( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	struct xml_info info;
	const char *res;

	res = xml_find_named( item_or_container, "res", &info );
	if ( res != NULL )
	{
		char duration[16];

		if ( xml_get_named_attribute( res, "duration", duration, sizeof( duration ) ) &&
		     IsValuableInfo( duration ) )
		{
			unsigned seconds = 0, x = 0;
			char *s = duration;

			/*TRACE(( "duration = \"%s\"\n", duration ));*/

			for (;;)
			{
				char ch = *s++;
				if ( ch >= '0' && ch <= '9' )
				{
					x *= 10;
					x += (ch - '0');
				}
				else if ( ch == ':' )
				{
					seconds += x;
					seconds *= 60;
					x = 0;
				}
				else if ( ch == '.' )
				{
					int ms = atoi( s );
					if ( ms >= 500 ) x++;
					break;
				}
				else if ( ch == '\0' )
				{
					break;
				}
				else
				{
					TRACE(( "*** GetDuration(): \"%s\" is not a known duration format\n", duration ));
					return NULL;
				}
			}

			seconds += x;
			/*TRACE(( "seconds = %u\n", seconds ));*/
			if ( seconds > 0 )
			{
				if ( value != NULL )
				{
					if ( seconds >= 3600 )
					{
						if ( sizeof_value > 8 )
						{
							size_t n = (size_t)sprintf( value, "%u:%02u:%02u", seconds / 3600, (seconds % 3600) / 60, seconds % 60 );
							ASSERT( n < sizeof_value );
							return value;
						}
					}
					else
					{
						if ( sizeof_value > 5 )
						{
							size_t n = (size_t)sprintf( value, "%u:%02u", seconds / 60, seconds % 60 );
							ASSERT( n < sizeof_value );
							return value;
						}
					}
				}
				else
				{
					if ( seconds >= 3600 )
						return g_strdup_printf( "%u:%02u:%02u", seconds / 3600, (seconds % 3600) / 60, seconds % 60 );
					else
						return g_strdup_printf( "%u:%02u", seconds / 60, seconds % 60 );
				}
			}
		}
	}

	return NULL;
}

gchar *GetGenre( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	return GetInfo( item_or_container, "upnp:genre", value, sizeof_value );
}

gchar *GetOriginalTrackNumber( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	return GetInfo( item_or_container, "upnp:originalTrackNumber", value, sizeof_value );
}

gchar *GetTitle( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	gchar *title = GetInfo( item_or_container, "dc:title", value, sizeof_value );
	if ( title == NULL )
	{
		if ( value != NULL )
		{
			if ( sizeof_value > 1 )
				strcpy( value, "?" );
		}
		else
		{
			title = g_strdup( "?" );
		}
	}

	return title;
}

gchar *GetTitleAndArtist( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	gchar *title = GetTitle( item_or_container, value, sizeof_value );
	if ( title != NULL )
	{
		if ( value != NULL )
		{
			size_t n = strlen( value );
			value += n;
			sizeof_value -= n;

			if ( sizeof_value > 4 && GetArtist( item_or_container, value + 3, sizeof_value - 3 ) != NULL )
			{
				*value++ = ' ';
				*value++ = '-';
				*value++ = ' ';
			}
		}
		else
		{
			gchar *artist = GetArtist( item_or_container, NULL, 0 );
			if ( artist != NULL )
			{
				gchar *tmp = g_strconcat( title, " - ", artist, NULL );
				g_free( artist );
				g_free( title );
				title = tmp;
			}
		}
	}

	return title;
}

gchar *GetYear( const gchar *item_or_container, char *value, size_t sizeof_value )
{
	char date[32], *s;

	if ( xml_get_named( item_or_container, "dc:date", date, sizeof( date ) ) != NULL &&
	     IsValuableInfo( date ) )
	{
		for ( s = date; *s != '\0'; s++ )
		{
			if ( s != date && isdigit( s[-1] ) ) continue;

			if ( isdigit( s[0] ) && isdigit( s[1] ) && isdigit( s[2] ) && isdigit( s[3] ) && !isdigit( s[4] ) )
			{
				s[4] = '\0';
				return ( value != NULL ) ? (( sizeof_value > 4 /* strlen( s ) */ ) ? strcpy( value, s ) : NULL) : g_strdup( s );
			}
		}

		TRACE(( "*** GetYear(): \"%s\" has not a known date format\n", date ));
	}

	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

#define INITIAL_ALBUM_ART_SIZE  64*1024

struct size_prepared_data
{
	gint image_size;
	gboolean shrink_only;
};

static void size_prepared( GdkPixbufLoader *loader, gint width, gint height, struct size_prepared_data *data )
{
	gint image_width, image_height;

	if ( width >= height )
	{
		image_width  = data->image_size;
		image_height = (height * image_width + width/2) / width;
	}
	else
	{
		image_height = data->image_size;
		image_width  = (width * image_height + height/2) / height;
	}

	TRACE(( "current_size_prepared( %d, %d ) => %d, %d\n", width, height, image_width, image_height ));

	if ( width - image_width > ALBUM_ART_SIZE_TOLERANCE ||
	     (!data->shrink_only && image_width - width > ALBUM_ART_SIZE_TOLERANCE) )
	{
		TRACE(( "gdk_pixbuf_loader_set_size( %d, %d )\n", image_width, image_height ));
		gdk_pixbuf_loader_set_size( loader, image_width, image_height );
	}
}

gchar *GetAlbumArtURI( const gchar *item_or_container )
{
	gchar *albumArtURI = GetInfo( item_or_container, "upnp:albumArtURI", NULL, 0 );
	if ( albumArtURI == NULL && Settings->lastfm.active && Settings->lastfm.album_art )
	{
		gchar *artist = GetArtist( item_or_container, NULL, 0 );
		gchar *album = GetAlbum( item_or_container, NULL, 0 );
		albumArtURI = lastfm_album_get_info_url( artist, album );
		g_free( album );
		g_free( artist );
	}

	return albumArtURI;
}

static gboolean get_album_art( const char *albumArtURI, char **content_type, char **content, size_t *content_length, GError **error )
{
	gboolean result = http_get( albumArtURI, INITIAL_ALBUM_ART_SIZE, content_type, content, content_length, error );
	TRACE(( "get_album_art(): http_get( %s ) = %s\n", albumArtURI, (result) ? "TRUE" : "FALSE" ));
	if ( !result ) { upnp_free_string( *content ); *content = NULL; }
	return result;
}

gboolean GetAlbumArt( const char *albumArtURI, char **content_type, char **content, size_t *content_length, GError **error )
{
	char *_content = NULL, *s;
	gboolean result = FALSE;

	TRACE(( "GetAlbumArt( %s )...\n", (albumArtURI != NULL) ? albumArtURI : "<NULL>" ));
	ASSERT( albumArtURI != NULL );

	if ( is_lastfm_url( albumArtURI ) )
	{
		gchar *url = (gchar *)albumArtURI;
		albumArtURI = NULL;

		if ( http_get( url, 0, NULL, &_content, NULL, NULL ) )
		{
			char *xml = _content;
			char *name;

			xml = xml_unbox( &xml, &name, NULL );
			if ( xml != NULL && strcmp( name, "lfm" ) == 0 )
			{
				xml = xml_unbox( &xml, &name, NULL );
				if ( xml != NULL && strcmp( name, "album" ) == 0 )
				{
					xml_iter iter;
					for ( xml = xml_first( xml, &iter ); xml != NULL; xml = xml_next( &iter ) )
					{
						char *value = xml_unbox( &xml, &name, NULL );
						if ( strcmp( name, "image" ) == 0 && IsValuableInfo( value ) )
						{
							albumArtURI = value;
							TRACE(( "  albumArtURI := %s\n", albumArtURI ));
						}
					}
				}
			}
		}

		if ( albumArtURI == NULL )
		{
			upnp_free_string( _content );
			return FALSE;
		}
	}
	else if ( (s = strstr( albumArtURI, "/cgi-bin/O" )) != NULL )
	{
		GString *_albumArtURI = g_string_new( albumArtURI );
		ASSERT( _albumArtURI != NULL );
		if ( _albumArtURI != NULL )
		{
			gssize pos = (s - albumArtURI) + 1;
			const char *ext;

			g_string_insert_c( _albumArtURI, pos++, 'a' );
			strncpy( _albumArtURI->str + pos, "lbumart", 7 );
			pos += 8;

			s   = strchr( _albumArtURI->str + pos, '/' );
			ext = strrchr( _albumArtURI->str + pos, '.' );
			if ( s != NULL && ext != NULL && ext > s )
			{
				strcpy( s, ext );

				TRACE(( "_albumArtURI = \"%s\"\n", _albumArtURI->str ));
				result = get_album_art( _albumArtURI->str, content_type, content, content_length, NULL );
			}

			g_string_free( _albumArtURI, TRUE );
		}
	}
	else if ( (s = strstr( albumArtURI, "/cgi-bin/W" )) != NULL )
	{
		gchar *_albumArtURI = g_strdup( albumArtURI );
		ASSERT( _albumArtURI != NULL );
		if ( _albumArtURI != NULL )
		{
			strcpy( _albumArtURI + (s - albumArtURI), "/albumart" );
			strcat( _albumArtURI, strrchr( albumArtURI, '/' ) );

			TRACE(( "_albumArtURI = \"%s\"\n", _albumArtURI ));
			result = get_album_art( _albumArtURI, content_type, content, content_length, NULL );

			g_free( _albumArtURI );
		}
	}
	else if ( (s = strstr( albumArtURI, "?scale=" )) != NULL && s[7] >= '0' && s[7] <= '9' )
	{
		gchar *_albumArtURI = g_strdup( albumArtURI );
		ASSERT( _albumArtURI != NULL );
		if ( _albumArtURI != NULL )
		{
			strcpy( _albumArtURI + (s - albumArtURI) + 7, "ORG" );

			TRACE(( "_albumArtURI = \"%s\"\n", _albumArtURI ));
			result = get_album_art( _albumArtURI, content_type, content, content_length, NULL );

			g_free( _albumArtURI );
		}
	}

	if ( !result )
	{
		result = get_album_art( albumArtURI, content_type, content, content_length, error );
	}

	upnp_free_string( _content );
	return result;
}

gboolean SetAlbumArt( GtkImage *image, gint image_size, gboolean shrink_only, const char *albumArtURI, char *content_type, char *content, size_t content_length, GError **error )
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbufLoader *loader;

	TRACE(( "Content-Type = %s\n", content_type ));
	TRACE(( "Content-Length = %u\n", (unsigned int)content_length ));

	/* Get GdkPixbuf */

	if ( content != NULL && content_length > 0 )
	{
		if ( strcmp( content_type, "application/octet-stream" ) == 0 )
		{
			const char *ext = strrchr( albumArtURI, '.' );
			if ( ext != NULL )
			{
				if ( stricmp( ext, ".gif" ) == 0 )
					content_type = "image/gif";
				else if ( stricmp( ext, ".jpe" ) == 0 || stricmp( ext, ".jpeg" ) == 0 || stricmp( ext, ".jpg" ) == 0 )
					content_type = "image/jpeg";
				else if ( stricmp( ext, ".png" ) == 0 )
					content_type = "image/png";
				else if ( stricmp( ext, ".tif" ) == 0 || stricmp( ext, ".tiff" ) == 0 )
					content_type = "image/tiff";
			}
		}

		/*image_width = image_height = 0;*/

		loader = gdk_pixbuf_loader_new_with_mime_type( content_type, error );
		TRACE(( "gdk_pixbuf_loader_new_with_mime_type( \"%s\" ) = %p\n", content_type, (void *)loader ));
		if ( loader != NULL )
		{
			struct size_prepared_data data;

			/* Scale image */
			data.image_size = image_size;
			data.shrink_only = shrink_only;
			g_signal_connect( G_OBJECT(loader), "size-prepared", G_CALLBACK(size_prepared), &data );

		#ifdef MAEMO
			/* Workaround for bug in GtkPixbufLoader (GTK+ 2.10.12) */
			while ( content_length > 0 )
			{
				/* gdk_pixbuf_new_from_file() uses a buffer of 4096 bytes, so we do, too */
				gsize length = (content_length > 4096) ? 4096 : content_length;

				if ( !gdk_pixbuf_loader_write( loader, (guchar*)content, length, error ) )
					break;

				content += length;
				content_length -= length;
			}
			if ( content_length == 0 )
		#else
			if ( gdk_pixbuf_loader_write( loader, (guchar*)content, (gsize)content_length, error ) )
		#endif
			{
				if ( gdk_pixbuf_loader_close( loader, error ) )
				{
					pixbuf = gdk_pixbuf_loader_get_pixbuf( loader );
					if ( pixbuf != NULL )
					{
						g_object_ref( pixbuf );
					}
					else
					{
						TRACE(( "*** SetAlbumArt(): gdk_pixbuf_loader_get_pixbuf() failed\n" ));
						g_set_error( error, 0, 0, "gdk_pixbuf_loader_get_pixbuf() failed" );
					}
				}
				else
				{
					TRACE(( "*** SetAlbumArt(): gdk_pixbuf_loader_close() failed\n" ));
				}
			}
			else
			{
				TRACE(( "*** SetAlbumArt(): gdk_pixbuf_loader_write() failed\n" ));
			}

			g_object_unref( loader );  /* Free pixbuf loader */
		}
		else
		{
			TRACE(( "*** SetAlbumArt(): gdk_pixbuf_loader_new_with_mime_type( \"%s\" ) failed\n", content_type ));
		}

		if ( error != NULL && *error != NULL )
		{
			TRACE(( "*** SetAlbumArt(): domain = %d, code = %d, message = %s\n", (int)(*error)->domain, (*error)->code, (*error)->message ));
		}
	}

	/* Set GtkImage to GdkPixbuf */

	if ( pixbuf != NULL )
	{
		TRACE(( "pixbuf->ref_count = %d\n", G_OBJECT(pixbuf)->ref_count ));
		ASSERT( G_OBJECT(pixbuf)->ref_count == 1 );

		gtk_image_set_from_pixbuf( image, pixbuf );
		g_object_unref( pixbuf );
		return TRUE;
	}

	gtk_image_set_from_stock( image, GTK_STOCK_MISSING_IMAGE, GTK_ICON_SIZE_BUTTON );
	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

static int compare_devices( const void *elem1, const void *elem2 )
{
	struct device_list_entry *dle1 = (struct device_list_entry *)elem1;
	struct device_list_entry *dle2 = (struct device_list_entry *)elem2;
	const upnp_device *dev1 = dle1->device;
	const upnp_device *dev2 = dle2->device;

	int result;

	result = g_utf8_collate( dev1->friendlyName, dev2->friendlyName );
	if ( result == 0 )
	{
		upnp_string value;

		if ( Linn_Volkano_ProductId( dev1, &value, NULL ) )
		{
			dle1->product_id = strtoul( value, NULL, 10 );
			upnp_free_string( value );
		}
		if ( Linn_Volkano_ProductId( dev2, &value, NULL ) )
		{
			dle2->product_id = strtoul( value, NULL, 10 );
			upnp_free_string( value );
		}

		result = dle1->product_id - dle2->product_id;
		if ( result == 0 )
		{
			result = strcmp( dev1->UDN, dev2->UDN );
		}
	}

	return result;
}

struct device_list_entry *GetServerList( int *n )
{
	struct device_list_entry *list;
	upnp_device *sr;
	int x, i = 0;

	x = upnp_num_devices( upnp_serviceId_ContentDirectory );
	list = g_new0( struct device_list_entry, x + 1 );

	for ( sr = upnp_get_first_device( upnp_serviceId_ContentDirectory ); sr != NULL; sr = upnp_get_next_device( sr, upnp_serviceId_ContentDirectory ) )
	{
		list[i++].device = sr;
	}
	ASSERT( i == x );

	qsort( list, i, sizeof( struct device_list_entry ), compare_devices );

	if ( n != NULL ) *n = x;
	return list;
}

struct device_list_entry *GetRendererList( int *n )
{
	struct device_list_entry *list;
	upnp_device *sr;
	int x, i = 0;

	x = upnp_num_av_transport();
	list = g_new0( struct device_list_entry, x + 1 );

	for ( sr = upnp_get_first_av_transport(); sr != NULL; sr = upnp_get_next_av_transport( sr ) )
	{
		list[i++].device = sr;
	}
	ASSERT( i == x );

	qsort( list, i, sizeof( struct device_list_entry ), compare_devices );

	if ( n != NULL ) *n = x;
	return list;
}

/*-----------------------------------------------------------------------------------------------*/
