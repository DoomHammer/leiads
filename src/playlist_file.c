/*
	playlist_file.c
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

#define PLAYLIST_FILE_EXTENSION    ".xml"
#define PLAYLIST_FILE_VERSION      3
#define PLAYLIST_FILE_VERSION_STR  "3"

/*-----------------------------------------------------------------------------------------------*/

struct MediaServer
{
	struct MediaServer *next;
	gchar *deviceType, *UDN, *friendlyName;
	GString *URLBase;
};

static struct MediaServer *get_media_server( const upnp_device *sr )
{
	struct MediaServer *result = g_new0( struct MediaServer, 1 );
	ASSERT( result != NULL );

	if ( result != NULL )
	{
		const char *s;

		result->deviceType = g_strdup( sr->deviceType );
		result->UDN = g_strdup( sr->UDN );
		result->friendlyName = g_strdup( sr->friendlyName );

		ASSERT( sr->URLBase != NULL );
		s = strchr( sr->URLBase + 7, '/' );    /* first '/' */
		ASSERT( s != NULL );
		if ( s != NULL ) s++; else s = strrchr( sr->URLBase, '\0' );
		result->URLBase = g_string_new_len( sr->URLBase, s - sr->URLBase );
	}

	return result;
}

static void free_media_server( struct MediaServer *media_server )
{
	ASSERT( media_server != NULL );

	if ( media_server != NULL )
	{
		if ( media_server->URLBase != NULL ) g_string_free( media_server->URLBase, TRUE );
		if ( media_server->friendlyName != NULL ) g_free( media_server->friendlyName );
		if ( media_server->UDN != NULL ) g_free( media_server->UDN );
		if ( media_server->deviceType != NULL ) g_free( media_server->deviceType );

		g_free( media_server );
	}
}

/*-----------------------------------------------------------------------------------------------*/

static struct MediaServer *LoadPlaylistMediaServer( char *content )
{
	/* Add "MediaServer" item to the media server list */
	struct MediaServer *media_server = g_new0( struct MediaServer, 1 );
	struct xml_info info;

	ASSERT( media_server != NULL );

	for (;;)
	{
		info.content = xml_unbox( &content, &info.name, NULL );
		if ( info.content == NULL ) break;

		if ( strcmp( info.name, "deviceType" ) == 0 )
			media_server->deviceType = g_strdup( info.content );
		else if ( strcmp( info.name, "UDN" ) == 0 )
			media_server->UDN = g_strdup( info.content );
		else if ( strcmp( info.name, "friendlyName" ) == 0 )
			media_server->friendlyName = g_strdup( info.content );
		else if ( strcmp( info.name, "URLBase" ) == 0 )
			media_server->URLBase = g_string_new( info.content );
	}

	return media_server;
}

static gchar *LoadPlaylistTrack( char *content, struct MediaServer *media_server_list, struct MediaServer **playlist_server_list )
{
	struct xml_info info;
	GString *item;
	gchar *res;

	/* Scan for "DIDL-Lite"... */
	for (;;)
	{
		info.content = xml_unbox( &content, &info.name, NULL );
		if ( info.content == NULL ) return NULL;  /* No "DIDL-Lite" found */

		if ( strcmp( info.name, "MetaData" ) == 0 )
			content = info.content;
		else if ( strcmp( info.name, "DIDL-Lite" ) == 0 )
			break;  /* found! */
	}

	/* Get "res" item */
	res = xml_get_named_str( info.content, "res" );
	/*if ( res == NULL ) res = xml_get_named( info.content, "ns:res" );*/
	if ( res == NULL ) return NULL;
	/*TRACE(( "$$$$$$$ res = \"%s\"\n", res ));*/

	/* Playlist server list available? (since version 2 of the playlist file format) */
	if ( *playlist_server_list != NULL )
	{
		struct MediaServer *playlist_server;

		/* Get corresponding "MediaServer" entry */
		for ( playlist_server = *playlist_server_list; playlist_server != NULL; playlist_server = playlist_server->next )
		{
			if ( strncmp( res, playlist_server->URLBase->str, playlist_server->URLBase->len ) == 0 )
			{
				struct MediaServer *media_server;

				/* Is there a media server available with this UDN? */
				for ( media_server = media_server_list; media_server != NULL; media_server = media_server->next )
				{
					if ( strcmp( playlist_server->UDN, media_server->UDN ) == 0 )
						break;  /* Yes! */
				}

				if ( media_server == NULL )
				{
					TRACE(( "$$$ LoadPlaylistTrack(): Media Server \"%s\" for URL \"%s\" is not available!\n", playlist_server->friendlyName, res ));
					g_free( res );
					return NULL;
				}
				g_free( res );

				item = g_string_new( info.content );

				if ( !g_string_equal( playlist_server->URLBase, media_server->URLBase ) )
				{
					/* The IP address has changed, so replace all BaseURLs within "Track" item */
					for (;;)
					{
						gssize pos;
						char *s;

						s = strstr( item->str, playlist_server->URLBase->str );
						if ( s == NULL ) break;  /* done */

						pos = s - item->str;
						g_string_erase( item, pos, playlist_server->URLBase->len );
						g_string_insert( item, pos, media_server->URLBase->str );
					}
				}

				return g_string_free( item, FALSE );
			}
		}
	}

	g_free( res );

	return g_strdup( info.content );
}

void append_audio_broadcast_item( GString *content_str, const char *title, const char *res, const char *album_art_uri, const char *genre )
{
	char id[32+4];
	md5cpy( id, res );

	if ( title == NULL ) title = res;

	g_string_append_printf( content_str, "<item id=\"leiads:%s\">", id );

	xml_string_append_boxed( content_str, "dc:title", title, NULL );
	if ( res != NULL && *res != '\0' )
		xml_string_append_boxed( content_str, "res", res, NULL );
	if ( album_art_uri != NULL && *album_art_uri != '\0' )
		xml_string_append_boxed( content_str, "upnp:albumArtURI", album_art_uri, NULL );
	if ( genre != NULL && *genre != '\0' )
		xml_string_append_boxed( content_str, "upnp:genre", genre, NULL );

	g_string_append( content_str, "<upnp:class>object.item.audioItem.audioBroadcast</upnp:class></item>" );
}

GString *load_m3u_playlist( char *s, int *tracks )
{
	GString *media_data = g_string_new( "" );
	gchar *title = NULL;

	/* Skip header */
	if ( strncmp( s, "#EXTM3U", 7 ) == 0 ) s += 7;

	for (;;)
	{
		char *t;

		while ( *s != '\0' && (unsigned char)*s <= ' ' ) s++;
		if ( *s == '\0' ) break;

		t = s;
		while ( *t != '\0' && *t != '\r' && *t != '\n' ) t++;
		if ( *t != '\0' ) *t++ = '\0';

		if ( strncmp( s, "#EXTINF", 7 ) == 0 )
		{
			g_free( title );
			title = NULL;

			s += 7;
			if ( *s == ':' )
			{
				s = strchr( s, ',' );
				if ( s != NULL )
				{
					if ( g_utf8_validate( ++s, -1, NULL ) )
						title = g_strdup( s );
					else
						title = g_convert( s, -1, "UTF-8", "ISO-8859-1", NULL, NULL, NULL );
				}
			}
		}
		else if ( strncmp( s, "http://", 7 ) == 0 )
		{
			append_audio_broadcast_item( media_data, title, s, NULL, NULL );
			(*tracks)++;
		}
		else
		{
			g_string_free( media_data, TRUE );
			media_data = NULL;
			break;
		}

		s = t;
	}

	g_free( title );
	return media_data;
}

GString *load_dpl_playlist( char *s, int *tracks, int *tracks_not_added )
{
	struct MediaServer *media_server_list, *playlist_server_list;
	GString *media_data = NULL;
	const upnp_device *sr;
	struct xml_info info;
	int track_count;

	/* Create media server list */
	media_server_list = NULL;
	for ( sr = upnp_get_first_device( upnp_serviceId_ContentDirectory ); sr != NULL; sr = upnp_get_next_device( sr, upnp_serviceId_ContentDirectory ) )
	{
		struct MediaServer *media_server = get_media_server( sr );
		ASSERT( media_server != NULL );
		if ( media_server != NULL )
		{
			media_server->next = media_server_list;
			media_server_list = media_server;
		}
	}

	/* Create (empty) playlist media server list */
	playlist_server_list = NULL;

	/* Count "Track" items */
	track_count = 0;
	for ( info.next = s; xml_get_info( info.next, &info ); )
	{
		if ( info.end_of_name - info.name == 5 && strncmp( info.name, "Track", 5 ) == 0 )
			track_count++;
		else if ( info.end_of_name - info.name == 10 && strncmp( info.name, "linn:Track", 10 ) == 0 )
			track_count++;
	}

	/* Create media play list */
	TRACE(( "LoadPlaylist(): track_count = %d\n", track_count ));
	media_data = g_string_sized_new( track_count * 1024 );
	ASSERT( media_data != NULL );

	*tracks_not_added = 0;

	/* Scan for "MediaServer" and "Track" items */
	for ( info.next = s;;)
	{
		info.content = xml_unbox( &info.next, &info.name, NULL );
		if ( info.content == NULL ) break;  /* finished */

		if ( strcmp( info.name, "MediaServer" ) == 0 )
		{
			struct MediaServer *media_server = LoadPlaylistMediaServer( info.content );

			ASSERT( media_server != NULL );
			TRACE(( "LoadPlaylist(): Media Server \"%s\"\n", media_server->friendlyName ));

			media_server->next = playlist_server_list;
			playlist_server_list = media_server;
		}
		else if ( strcmp( info.name, "Track" ) == 0 || strcmp( info.name, "linn:Track" ) == 0 )
		{
			gchar *item = LoadPlaylistTrack( info.content, media_server_list, &playlist_server_list );
			if ( item != NULL )
			{
				g_string_append( media_data, item );
				g_free( item );

				(*tracks)++;
			}
			else
			{
				(*tracks_not_added)++;
			}
		}
	}

	/* Free media server lists */
	while ( playlist_server_list != NULL )
	{
		struct MediaServer *next = playlist_server_list->next;
		free_media_server( playlist_server_list );
		playlist_server_list = next;
	}
	while ( media_server_list != NULL )
	{
		struct MediaServer *next = media_server_list->next;
		free_media_server( media_server_list );
		media_server_list = next;
	}

	return media_data;
}

struct xml_content *LoadPlaylist( const char *filename, int *tracks_not_added, GError **error )
{
	GString *media_data = NULL;
	gchar *buf = NULL;
	int tracks = 0;

	ASSERT( filename != NULL );
	TRACE(( "LoadPlaylist( \"%s\" )...\n", filename ));

	if ( g_file_get_contents( filename, &buf, NULL, error ) )
	{
		struct xml_info info;
		char *s = buf;

		ASSERT( buf != NULL );

		/* Verify that it is a valid playlist file */
		if ( *s == '#' || *s == 'h' )
		{
			media_data = load_m3u_playlist( s, &tracks );
		}
		else if ( (s = xml_unbox( &s, &info.name, &info.attributes )) != NULL &&
			 (strcmp( info.name, "PlaylistData" ) == 0 || strcmp( info.name, "linn:Playlist" ) == 0) )
		{
			media_data = load_dpl_playlist( s, &tracks, tracks_not_added );
		}

		g_free( buf );

		if ( media_data == NULL )
		{
			g_set_error( error, 0, 0, Text(PLAYLIST_NOT_A_VALID_FILE) );
		}

		TRACE(( "LoadPlaylist(): tracks_not_added = %d\n", *tracks_not_added ));
	}

	TRACE(( "LoadPlaylist() = %p\n", (void*)media_data ));
	return xml_content_from_string( media_data, tracks );
}

void HandleNotAddedTracks( int tracks_not_added )
{
	if ( tracks_not_added )
	{
		gchar *description = g_strdup_printf( Text(PLAYLIST_TRACKS_NOT_ADDED), tracks_not_added );
		message_dialog( GTK_MESSAGE_WARNING, description );
		g_free( description );
	}
}

/*-----------------------------------------------------------------------------------------------*/

gboolean SavePlaylist( const char *filename, GError **error )
{
	gboolean result = FALSE;
	FILE *fp;

	ASSERT( filename != NULL );
	TRACE(( "SavePlaylist( \"%s\" )...\n", filename ));

	fp = CreatePlaylistFile( filename, TRUE, error );
	if ( fp != NULL )
	{
		int i;

		for ( i = 0;; i++ )
		{
			gchar *item = GetTrackItem( i );
			if ( item == NULL ) break;

			result = WritePlaylistItem( fp, item, error );
			if ( !result ) break;

			g_free( item );
		}

		result = ClosePlaylistFile( fp, error );
		if ( !result ) remove( filename );
	}

	TRACE(( "SavePlaylist() = %s\n", result ? "TRUE" : "FALSE" ));
	return result;
}

/*-----------------------------------------------------------------------------------------------*/

#define LEIA_PATTERN  1
#define LINN_PATTERN  2
#define M3U_PATTERN   4

GtkFileFilter *file_chooser_add_playlist_filter( GtkFileChooser *chooser, gint pattern )
{
	static const char leia_pattern[] = "*"PLAYLIST_FILE_EXTENSION;
	static const char linn_pattern[] = "*.dpl";
	static const char m3u_pattern[] = "*.m3u";

	GtkFileFilter *filter = gtk_file_filter_new();
	GString *filter_name = g_string_sized_new( 64 );

	if ( filter_name != NULL )
	{
		if ( pattern == LEIA_PATTERN )
			g_string_printf( filter_name, Text(PLAYLIST_FILTER_NAME_1), NAME );
		else if ( pattern == LINN_PATTERN )
			g_string_printf( filter_name, Text(PLAYLIST_FILTER_NAME_1), "Linn" );
		else if ( pattern == M3U_PATTERN )
			g_string_printf( filter_name, Text(PLAYLIST_FILTER_NAME_1), "M3U" );
		else
			g_string_append( filter_name, Text(PLAYLIST_FILTER_NAME) );

		g_string_append( filter_name, " (" );
		if ( (pattern & LEIA_PATTERN) != 0 )
		{
			g_string_append( filter_name, leia_pattern );
			if ( (pattern & ~LEIA_PATTERN) != 0 )
				g_string_append( filter_name, ";" );
		}
		if ( (pattern & LINN_PATTERN) != 0 )
		{
			g_string_append( filter_name, linn_pattern );
			if ( (pattern & M3U_PATTERN) != 0 )
				g_string_append( filter_name, ";" );
		}
		if ( (pattern & M3U_PATTERN) != 0 )
		{
			g_string_append( filter_name, m3u_pattern );
		}
		g_string_append( filter_name, ")" );

		gtk_file_filter_set_name( filter, filter_name->str );

		g_string_free( filter_name, TRUE );
	}

	if ( (pattern & LEIA_PATTERN) != 0 )
		gtk_file_filter_add_pattern( filter, leia_pattern );
	if ( (pattern & LINN_PATTERN) != 0 )
		gtk_file_filter_add_pattern( filter, linn_pattern );
	if ( (pattern & M3U_PATTERN) != 0 )
		gtk_file_filter_add_pattern( filter, m3u_pattern );

	gtk_file_chooser_add_filter( chooser, filter );
	return filter;
}

gchar *ChoosePlaylistFile( GtkWindow *parent, const char *current_name, gchar **current_folder )
{
	gchar *filename = NULL;

	GtkFileChooser *chooser;
	GtkFileFilter *filter;

	if ( current_name == NULL )
	{
		chooser = GTK_FILE_CHOOSER(new_file_chooser_dialog( Text(PLAYLIST_LOAD_CAPTION), parent, GTK_FILE_CHOOSER_ACTION_OPEN ));
	}
	else
	{
		gchar *current_name_with_extension = g_strconcat( current_name, PLAYLIST_FILE_EXTENSION, NULL );

		chooser = GTK_FILE_CHOOSER(new_file_chooser_dialog( Text(PLAYLIST_SAVE_CAPTION), parent, GTK_FILE_CHOOSER_ACTION_SAVE ));
		gtk_file_chooser_set_current_name( chooser, current_name_with_extension );
		gtk_file_chooser_set_do_overwrite_confirmation( chooser, TRUE );

		g_free( current_name_with_extension );
	}

	if ( current_folder != NULL && *current_folder != NULL )
		gtk_file_chooser_set_current_folder( chooser, *current_folder );

	if ( current_name == NULL )
	{
		filter = file_chooser_add_playlist_filter( chooser, LEIA_PATTERN | LINN_PATTERN | M3U_PATTERN );
		file_chooser_add_playlist_filter( chooser, LEIA_PATTERN );
		file_chooser_add_playlist_filter( chooser, LINN_PATTERN );
		file_chooser_add_playlist_filter( chooser, M3U_PATTERN );
	}
	else
	{
		filter = file_chooser_add_playlist_filter( chooser, LEIA_PATTERN );
	}
	gtk_file_chooser_set_filter( chooser, filter );

	if ( gtk_dialog_run( GTK_DIALOG(chooser) ) == GTK_RESPONSE_OK )
	{
		filename = gtk_file_chooser_get_filename( chooser );

		if ( filename != NULL )
		{
			if ( current_folder != NULL )
			{
				gchar *cf = gtk_file_chooser_get_current_folder( chooser );

				if ( *current_folder == NULL || strcmp( *current_folder, cf ) != 0 )
				{
					gchar *tmp = cf;
					cf = *current_folder;
					*current_folder = tmp;
				}

				g_free( cf );
			}

			if ( current_name != NULL )
			{
				int filename_len = strlen( filename );
				if ( filename_len < 4 || strcmp( filename + filename_len - 4, PLAYLIST_FILE_EXTENSION ) != 0 )
				{
					gchar *filename_xml = g_strconcat( filename, PLAYLIST_FILE_EXTENSION, NULL );
					g_free( filename );
					filename = filename_xml;
				}
			}
		}
	}

	gtk_widget_destroy( GTK_WIDGET(chooser) );
	return filename;
}

FILE *CreatePlaylistFile( const char *filename, gboolean include_server_list, GError **error )
{
	FILE *fp = fopen_utf8( filename, "wb" );
	if ( fp != NULL )
	{
	#if PLAYLIST_FILE_VERSION >= 2
		fputs( "<?xml version=\"1.0\" encoding=\"utf-8\"?>", fp );
		fwrite( beginPlaylistData, 1, strlen(beginPlaylistData) - 1, fp );
		fputs( " version=\"" PLAYLIST_FILE_VERSION_STR "\" generator=\"" NAME "\">", fp );

		if ( include_server_list )
		{
			const upnp_device *sr;

			for ( sr = upnp_get_first_device( upnp_serviceId_ContentDirectory ); sr != NULL; sr = upnp_get_next_device( sr, upnp_serviceId_ContentDirectory ) )
			{
				struct MediaServer *media_server = get_media_server( sr );
				ASSERT( media_server != NULL );
				if ( media_server != NULL )
				{
					fputs( "<MediaServer>", fp );
					if ( media_server->deviceType != NULL )
						file_put_string( fp, "deviceType", media_server->deviceType );
					if ( media_server->UDN != NULL )
						file_put_string( fp, "UDN", media_server->UDN );
					if ( media_server->friendlyName != NULL )
						file_put_string( fp, "friendlyName", media_server->friendlyName );
					if ( media_server->URLBase != NULL )
						file_put_string( fp, "URLBase", media_server->URLBase->str );
					fputs( "</MediaServer>", fp );

					free_media_server( media_server );
				}
			}
		}
	#else
		fputs( beginPlaylistData, fp );
	#endif
	}
	else
	{
		g_set_error( error, 0, 0, Text(ERRMSG_CREATE_FILE), filename );
	}

	return fp;
}

gboolean WritePlaylistItem( FILE *fp, const char *item, GError **error )
{
	gboolean result = (error != NULL && *error != NULL ) ? FALSE : TRUE;
	gchar *data, *playlist_data;
	int err;

	data = g_strconcat(
		"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">",
		item,
		"</DIDL-Lite>", NULL );

	playlist_data = xml_box( "Track", data, NULL );

	err = fputs( playlist_data, fp );
	if ( err < 0 )
	{
		g_set_error( error, 0, err, Text(ERRMSG_WRITE_FILE), "?" );
		result = FALSE;
	}

	g_free( playlist_data );
	g_free( data );

	return result;
}

gboolean ClosePlaylistFile( FILE *fp, GError **error )
{
	gboolean result = (error != NULL && *error != NULL ) ? FALSE : TRUE;

	if ( result )
	{
		int err = fputs( endPlaylistData, fp );
		if ( err < 0 )
		{
			g_set_error( error, 0, err, Text(ERRMSG_WRITE_FILE), "?" );
			result = FALSE;
		}
	}

	fclose( fp );

	return result;
}

/*-----------------------------------------------------------------------------------------------*/
