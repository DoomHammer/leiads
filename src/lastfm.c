/*
	lastfm.c
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

#define BITRATE    "128"
#define BUYLINKS   "1"

/*-----------------------------------------------------------------------------------------------*/

static const char *lastfm_keys[NUM_LASTFM_KEYS] =
	{ "similarartists", "fans", "globaltags", "library", "neighbours", "loved", "recommended" };

static const char history_filename[] = "lastfm_history.xml";
#define LASTFM_HISTORY_COUNT 100

const char lastfm_api_url[] = "http://ws.audioscrobbler.com/2.0/";
gchar *lastfm_name, *lastfm_key;

#if LASTFM

/*
	NOTE:
	=====

	The file "../../lastfm/api_key.h" contains a (non-public)
	Last.fm API Key & secret in the following format:

	static const char api_key[] = "...";
	static const char secret[]  = "...";

	The file "../../lastfm/api_key.h" is NOT part of this work and is
	therefore one of the prerequisites for building an executable of Leia
	with Last.fm support. If non-existing, simply create this file on your own
	and fill it with YOUR very own Last.fm API key & secret.

	If you don't have a Last.fm API account yet, you can apply for it at
	http://www.lastfm.de/api/account.

	The Last.fm API Terms of Service can be found at
	http://www.lastfm.de/api/tos.
*/

#include "../../lastfm/api_key.h"

#else

static const char *api_key = NULL;
static const char *secret  = NULL;

#endif

/*-----------------------------------------------------------------------------------------------*/

#include "../icons/16x16/lastfm.h"

void RegisterLastfmStockIcon( void )
{
#if 1
	register_stock_icon( LEIADS_STOCK_LASTFM, lastfm_16x16 );
#else
	GtkIconFactory *factory = get_icon_factory();
	GtkIconSet *icon_set = gtk_icon_set_new();

	register_sized_icon( icon_set, GTK_ICON_SIZE_SMALL_TOOLBAR, lastfm_16x16 );
	register_sized_icon( icon_set, GTK_ICON_SIZE_LARGE_TOOLBAR, lastfm_24x24 );
	register_sized_icon( icon_set, GTK_ICON_SIZE_BUTTON, lastfm_26x26 );

	gtk_icon_factory_add( factory, LEIADS_STOCK_LASTFM, icon_set );
	gtk_icon_set_unref( icon_set );
#endif
}

#define MAX_RGB 0xFFFF

static double Y( unsigned R, unsigned G, unsigned B )  /* Luminance */
{
	double result = 0.30 * R + 0.59 * G + 0.11 * B;
	TRACE(( "set_lastfm_image(): Y = %f\n", result ));
	return result;
}

gchar *GetLastfmLogoFilename( gboolean badge, gboolean small, const GdkColor *bg )
{
	GString *filename = g_string_new( (badge) ? "badge_" : "lastfm_" );
	const char *color;

	ASSERT( bg != NULL );

	if ( bg->red == MAX_RGB && bg->green == MAX_RGB && bg->blue == MAX_RGB )
		color = "red";
	else if ( Y( bg->red, bg->green, bg->blue ) >= (MAX_RGB+1) / 2 )
		color = "black";
	else
		color = "grey";

	g_string_append( filename, color );
	if ( small ) g_string_append( filename, "_small" );
	g_string_append( filename, (badge) ? ".gif" : ".png" );

	return g_string_free( filename, FALSE );
}

/*-----------------------------------------------------------------------------------------------*/

const char LastfmLeiaSessionKey[] = "leiads";

void SetLastfmSessionKey( const char *name, const char *key )
{
	g_free( lastfm_name );
	lastfm_name = g_strdup( name );

	g_free( lastfm_key );
	lastfm_key = g_strdup( key );
}

const char *GetLastfmSessionKey( void )
{
	return lastfm_key;
}

gchar *GetLastfmSessionKeyFromTrack( int track_index )
{
	gchar *item = GetTrackItem( track_index );
	gchar *sk = NULL;

	if ( item != NULL )
	{
		gchar *id = xml_get_named_attribute_str( item, "id" );
		if ( id != NULL )
		{
			if ( strncmp( id, "lastfm:", 7 ) == 0 )
			{
				char *s = strchr( id + 7, ':' );
				if ( s != NULL )
				{
					*s = '\0';
					sk = strcpy( id, id + 7 );
					id = NULL;
				}
			}

			g_free( id );
		}

		g_free( item );
	}

	return sk;
}

gboolean IsOurLastfmSession( const char *key )
{
	gboolean result = FALSE;

	if ( lastfm_key != NULL )
	{
		if ( key != NULL )
		{
			if ( strcmp( lastfm_key, key ) == 0 ) result = TRUE;
		}
		else
		{
			gchar *key = GetLastfmSessionKeyFromTrack( CurrentTrackIndex );
			if ( key != NULL )
			{
				result = IsOurLastfmSession( key );
				g_free( key );
			}
		}
	}

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

const char *GetLastfmRadioTitle( void )
{
	if ( Settings->lastfm.active && Settings->lastfm.username[0] )
	{
	/*
		From http://www.lastfm.de/api/radio:
		"Note: Due to licensing restrictions, you may not use the radio API on mobile telephones."
	*/

	#ifdef OSSO_SERVICE
		if ( strcmp( osso_product_hardware, "SU-18" ) != 0 && strcmp( osso_product_hardware, "<unknown>" ) != 0 &&
			 strcmp( osso_product_hardware, "RX-34" ) != 0 &&
			 strcmp( osso_product_hardware, "RX-44" ) != 0 && strcmp( osso_product_hardware, "RX-48" ) != 0 )
		{
			TRACE(( "GetLastfmRadioTitle(): No Nokia 770, N800, or N810\n" ));
			return NULL;
		}
	#endif

		return Text(LASTFM_RADIO);
	}

	return NULL;
}

static gchar *get_lastfm_station( const char *name, enum LastfmKey key, const char *value )
{
	ASSERT( key >= 0 && key < NUM_LASTFM_KEYS );
	return xml_box( name, value, "type", lastfm_keys[key], NULL );
}

const char *GetLastfmRadioItem( enum LastfmKey key, const char *value, GString *result )
{
	const char *radio = NULL, *label = NULL, *format = NULL;

	switch ( key )
	{
	case SimilarArtists:
		radio = Text(LASTFM_RADIO_SIMILAR_ARTISTS);
		label = Text(LASTFM_ARTIST);
		format = "artist/%s/similarartists";
		break;
	case Fans:
		radio = Text(LASTFM_RADIO_TOP_FANS);
		label = Text(LASTFM_ARTIST);
		format = "artist/%s/fans";
		break;
	case GlobalTags:
		radio = Text(LASTFM_RADIO_GLOBAL_TAG);
		label = Text(LASTFM_TAG);
		format = "globaltags/%s";
		break;
	case Library:
		radio = Text(LASTFM_RADIO_LIBRARY);
		label = Text(LASTFM_USER);
		format = "user/%s/library";
		break;
	case Neighbours:
		radio = Text(LASTFM_RADIO_NEIGHBOURS);
		label = Text(LASTFM_USER);
		format = "user/%s/neighbours";
		break;
	case Loved:
		radio = Text(LASTFM_RADIO_LOVED_TRACKS);
		label = Text(LASTFM_USER);
		format = "user/%s/loved";
		break;
	case Recommended:
		radio = Text(LASTFM_RADIO_RECOMMENDATION);
		label = Text(LASTFM_USER);
		format = "user/%s/recommended";
		break;
	}

	if ( radio != NULL )
	{
		if ( value != NULL )
		{
			gchar *s, *t;

			ASSERT( result != NULL );

			g_string_append( result, "<item id=\"lastfm://" );
			s = url_encoded_path( value );
			g_string_append_printf( result, format, s );
			g_free( s );
			g_string_append( result, "\">" );

			s = g_strconcat( value, " (", radio, ")", NULL );
			t = xml_box( "dc:title", s, NULL );
			g_free( s );
			g_string_append( result, t );
			g_free( t );

			s = get_lastfm_station( "lastfm:station", key, value );
			g_string_append( result, s );
			g_free( s );

			g_string_append( result,
				"<upnp:class>object.item.audioItem.audioBroadcast</upnp:class>"
				"</item>" );
		}
		else if ( result != NULL )
		{
			ASSERT( label != NULL );
			g_string_assign( result, label );
		}
	}

	return radio;
}

gboolean IsLastfmItem( const char *didl_item )
{
	char id[8];

	return ( xml_get_named_attribute( didl_item, "id", id, sizeof(id) ) != NULL &&
	         strncmp( id, "lastfm", 6 ) == 0 );
}

gchar *LastfmRadioPlaylistToUpnpPlaylist( const char *title_strip, char *trackList )
{
	struct xml_info info;
	gchar *server_address;
	GString *playlist;

	trackList = xml_find_named( trackList, "trackList", &info );
	if ( trackList == NULL ) return NULL;

	trackList = xml_unbox( &trackList, NULL, NULL );
	if ( trackList == NULL ) return NULL;

	playlist = g_string_new( "" );
	server_address = http_get_server_address( GetAVTransport() );

	if ( title_strip )
	{
		gchar *filename = BuildApplDataFilename( "lastfm.mp3" );
		gchar *encoded_filename = url_encoded_path( filename );
		gchar *location = g_strconcat( "http://", server_address, "/file//", encoded_filename, NULL );
		const char *identifier = "title_strip";

		TRACE(( "identifier = %s\n", identifier ));
		g_string_append_printf( playlist, "<item id=\"lastfm:%s:%s\">", LastfmLeiaSessionKey, identifier );

		xml_string_append_boxed( playlist, "dc:title", title_strip, NULL );
		/*xml_string_append_boxed( playlist, "dc:creator", NAME, NULL );*/
		xml_string_append_boxed( playlist, "res", location, "protocolInfo", "http-get:*:audio/mpeg:*", NULL );
		xml_string_append_boxed( playlist, "upnp:class", "object.item.audioItem.musicTrack", NULL );

		g_string_append( playlist, "</item>" );

		g_free( location );
		g_free( encoded_filename );
		g_free( filename );
	}

	for (;;)
	{
		char *name;
		char *track = xml_unbox( &trackList, &name, NULL );
		if ( track == NULL ) break;

		if ( strcmp( name, "track" ) == 0 )
		{
			gchar *location   = xml_get_named_str( track, "location" );
			gchar *title      = xml_get_named_str( track, "title" );
			gchar *identifier = xml_get_named_str( track, "identifier" );
			gchar *album      = xml_get_named_str( track, "album" );
			gchar *creator    = xml_get_named_str( track, "creator" );
			gchar *duration   = xml_get_named_str( track, "duration" );
			gchar *image      = xml_get_named_str( track, "image" );

			gchar *trackauth    = xml_get_named_str( track, "trackauth" );
			gchar *albumid      = xml_get_named_str( track, "albumid" );
			gchar *artistid     = xml_get_named_str( track, "artistid" );
			gchar *recording    = xml_get_named_str( track, "recording" );
			gchar *artistpage   = xml_get_named_str( track, "artistpage" );
			gchar *albumpage    = xml_get_named_str( track, "albumpage" );
			gchar *trackpage    = xml_get_named_str( track, "trackpage" );
			gchar *buyTrackURL  = xml_get_named_str( track, "buyTrackURL" );
			gchar *buyAlbumURL  = xml_get_named_str( track, "buyAlbumURL" );
			gchar *freeTrackURL = xml_get_named_str( track, "freeTrackURL" );
			gchar *explicit     = xml_get_named_str( track, "explicit" );
			gchar *loved        = xml_get_named_str( track, "loved" );

			TRACE(( "identifier = %s\n", ( identifier != NULL ) ? identifier : "<NULL>" ));
			g_string_append_printf( playlist, "<item id=\"lastfm:%s:%s\">", lastfm_key, ( identifier != NULL ) ? identifier : "?" );

			xml_string_append_boxed( playlist, "dc:title", title, NULL );
			xml_string_append_boxed( playlist, "dc:creator", creator, NULL );
			xml_string_append_boxed( playlist, "upnp:artist", creator, NULL );
			xml_string_append_boxed( playlist, "upnp:album", album, NULL );
			xml_string_append_boxed( playlist, "upnp:albumArtURI", image, NULL );

			if ( duration != NULL && duration[0] != '\0' )
			{
				unsigned long sec = strtoul( duration, NULL, 10 ) / 1000;
				g_free( duration );
				duration = g_strdup_printf( "%02lu:%02lu:%02lu", sec / 3600, (sec % 3600) / 60, sec % 60 );
			}

			name = g_strconcat( "http://", server_address, "/http//", location + 7, NULL );
			http_server_add_to_url_list( location );
			g_free( location );
			location = name;

			xml_string_append_boxed( playlist, "res", location, "duration", duration, "protocolInfo", "http-get:*:audio/mpeg:*", NULL );
			xml_string_append_boxed( playlist, "upnp:class", "object.item.audioItem.musicTrack", NULL );

			xml_string_append_boxed( playlist, "lastfm:trackauth",    trackauth, NULL );
			xml_string_append_boxed( playlist, "lastfm:albumid",      albumid, NULL );
			xml_string_append_boxed( playlist, "lastfm:artistid",     artistid, NULL );
			xml_string_append_boxed( playlist, "lastfm:recording",    recording, NULL );
			xml_string_append_boxed( playlist, "lastfm:artistpage",   artistpage, NULL );
			xml_string_append_boxed( playlist, "lastfm:albumpage",    albumpage, NULL );
			xml_string_append_boxed( playlist, "lastfm:trackpage",    trackpage, NULL );
			xml_string_append_boxed( playlist, "lastfm:buyTrackURL",  buyTrackURL, NULL );
			xml_string_append_boxed( playlist, "lastfm:buyAlbumURL",  buyAlbumURL, NULL );
			xml_string_append_boxed( playlist, "lastfm:freeTrackURL", freeTrackURL, NULL );
			xml_string_append_boxed( playlist, "lastfm:explicit",     explicit, NULL );
			xml_string_append_boxed( playlist, "lastfm:loved",        loved, NULL );

			g_string_append( playlist, "</item>" );

			g_free( loved );
			g_free( explicit );
			g_free( freeTrackURL );
			g_free( buyAlbumURL );
			g_free( buyTrackURL );
			g_free( trackpage );
			g_free( albumpage );
			g_free( artistpage );
			g_free( recording );
			g_free( artistid );
			g_free( albumid );
			g_free( trackauth );

			g_free( image );
			g_free( duration );
			g_free( creator );
			g_free( album );
			g_free( identifier );
			g_free( title );
			g_free( location );
		}
	}

	g_free( server_address );
	return g_string_free( playlist, FALSE );
}

/*-----------------------------------------------------------------------------------------------*/

static struct lastfm_history
{
	struct lastfm_history *next;
	enum LastfmKey key;
	gchar *value;
} *history;

static enum LastfmKey get_lastfm_key( const char *key )
{
	int i;

	ASSERT( key != NULL );

	if ( key != NULL )
	{
		for ( i = 0; i < NUM_LASTFM_KEYS; i++ )
		{
			if ( strcmp( key, lastfm_keys[i] ) == 0 )
				return (enum LastfmKey)i;
		}
	}

	return (enum LastfmKey)0;
}

static void append_lastfm( enum LastfmKey key, const char *value )
{
	struct lastfm_history *l = g_new( struct lastfm_history, 1 );
	l->key = key;
	l->value = g_strdup( value );
	list_append( &history, l );
}

struct xml_content *GetLastfmFolder( GError **error )
{
	struct lastfm_history *l;
	GString *content_str;
	int n;

	if ( GetLastfmSessionKey() == NULL )
	{
		upnp_string session = lastfm_auth_get_mobile_session( Settings->lastfm.username, Settings->lastfm.password, error );
		if ( session != NULL )
		{
			gchar *name = xml_get_named_str( session, "name" );  /* Username with correct case */
			gchar *key  = xml_get_named_str( session, "key" );

			TRACE(( "lastfm_auth_get_mobile_session() = %s,%s\n", name, key ));
			SetLastfmSessionKey( name, key );

			g_free( key );
			g_free( name );

			upnp_free_string( session );
		}
		else
		{
			TRACE(( "GetLastfmFolder(): lastfm_auth_get_mobile_session() failed\n" ));
			return NULL;
		}
	}

	ASSERT( GetLastfmSessionKey() != NULL );

	if ( history == NULL )
	{
		gchar *filename, *contents = NULL;

		filename = BuildUserConfigFilename( history_filename );
		g_file_get_contents( filename, &contents, NULL, NULL );
		g_free( filename );

		if ( contents != NULL )
		{
			char *s = contents;
			char *name;

			/* Verify that it is a valid settings file */
			if ( (s = xml_unbox( &s, &name, NULL )) != NULL && strcmp( name, "LastfmRadio" ) == 0 )
			{
				char *attribs, *key, *value;
				int n = 0;

				while ( (value = xml_unbox( &s, &name, &attribs )) != NULL )
				{
					if ( strcmp( name, "station" ) == 0 )
					{
						key = xml_unbox_attribute( &attribs, &name );
						if ( key != NULL && strcmp( name, "type" ) == 0 )
						{
							append_lastfm( get_lastfm_key( key ), value );
							if ( ++n == LASTFM_HISTORY_COUNT ) break;
						}
					}
				}
			}

			g_free( contents );
		}

		if ( history == NULL )
		{
			append_lastfm( GlobalTags, "Ambient" );
			append_lastfm( GlobalTags, "Chanson" );
			append_lastfm( GlobalTags, "Chillout" );
			append_lastfm( GlobalTags, "Classical" );
			append_lastfm( GlobalTags, "Country" );
			append_lastfm( GlobalTags, "Disco" );
			append_lastfm( GlobalTags, "Electronic" );
			append_lastfm( GlobalTags, "Hip-hop" );
			append_lastfm( GlobalTags, "Indie" );
			append_lastfm( GlobalTags, "Jazz" );
			append_lastfm( GlobalTags, "Metal" );
			append_lastfm( GlobalTags, "Pop" );
			append_lastfm( GlobalTags, "Rock" );
			append_lastfm( GlobalTags, "Schlager" );
			append_lastfm( GlobalTags, "Techno" );

			append_lastfm( Library, lastfm_name );
			append_lastfm( Neighbours, lastfm_name );
			append_lastfm( Loved, lastfm_name );
			append_lastfm( Recommended, lastfm_name );
		}
	}

	content_str = g_string_new( "" );
	n = 0;

	g_string_append_printf( content_str,
		"<item id=\"lastfm\"><dc:title>%s</dc:title><upnp:class>object.new.lastfmRadio</upnp:class></item>",
		Text(LASTFM_RADIO_NEW) );
	n++;

	for ( l = history; l != NULL; l = l->next )
	{
		VERIFY( GetLastfmRadioItem( l->key, l->value, content_str ) != NULL );
		n++;
	}

	return xml_content_from_string( content_str, n );
}

static gpointer save_lastfm_history( gpointer data )
{
	gchar *filename = BuildUserConfigFilename( history_filename );
	struct lastfm_history *l;
	FILE *fp;

	fp = fopen_utf8( filename, "wb" );
	if ( fp != NULL )
	{
		fputs( "<?xml version=\"1.0\" encoding=\"utf-8\"?>", fp );
		fputs( "<LastfmRadio xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\">", fp );

		for ( l = history; l != NULL; l = l->next )
		{
			gchar *s = get_lastfm_station( "station", l->key, l->value );
			fputs( s, fp );
			g_free( s );
		}

		fputs( "</LastfmRadio>", fp );
		fclose( fp );
	}
	else
	{
		GError *error = NULL;
		g_set_error( &error, 0, 0, "%s\n\n%s = %s", Text(ERRMSG_CREATE_FILE), Text(FILE_NAME), filename );
		HandleError( error, Text(ERROR_SAVE_LASTFM_HISTORY) );
	}

	return NULL;
}

static void add_to_lastfm_history( const char *key, const char *value )
{
	enum LastfmKey _key = get_lastfm_key( key );
	gchar *_value;
	struct lastfm_history *l;

	if ( history->key == _key && strcmp( history->value, value ) == 0 )
		return;

	_value = g_utf8_casefold( value, -1 );

	for ( l = history; l != NULL; l = l->next )
	{
		if ( l->key == _key )
		{
			gchar *l_value = g_utf8_casefold( l->value, -1 );
			int i = strcmp( l_value, _value );
			g_free( l_value );
			if ( i == 0 )
			{
				list_remove( &history, l );
				g_free( l->value );
				break;
			}
		}
	}

	g_free( _value );

	if ( l == NULL ) l = g_new( struct lastfm_history, 1 );

	l->key = _key;
	l->value = g_strdup( value );

	list_prepend( &history, l );

	g_thread_create_full( save_lastfm_history, NULL, 0, FALSE, FALSE, G_THREAD_PRIORITY_LOW, NULL );
}

gboolean AddToLastfmFolder( const char *didl_item )
{
	gchar *name, *attribs, *key, *value;
	gboolean result = FALSE;

	value = xml_get_named_with_attributes_str( didl_item, "lastfm:station", &attribs );
	if ( attribs != NULL && value != NULL )
	{
		char *s = attribs;
		key = xml_unbox_attribute( &s, &name );
		if ( key != NULL && strcmp( name, "type" ) == 0 )
		{
			add_to_lastfm_history( key, value );
			result = TRUE;
		}
	}

	g_free( value );
	g_free( attribs );

	return result;
}

/*-----------------------------------------------------------------------------------------------*/

static gchar *lastfm_url_v( const char *method, va_list marker )
{
	GString *url = g_string_new( lastfm_api_url );
	gchar *_url;

	ASSERT( method != NULL && *method != '\0' );
	g_string_append_printf( url, "?method=%s&api_key=%s", method, api_key );

	for (;;)
	{
		const char *key, *value;
		gchar *s;

		key = va_arg( marker, const char * );
		if ( key == NULL ) break;

		value = va_arg( marker, const char * );
		if ( value == NULL )
		{
			TRACE(( "* lastfm_url( %s ): value(%s) = NULL, aborting\n", method, key ));
			g_string_free( url, TRUE );
			url = NULL;
			break;
		}

		s = url_encoded( value );
		g_string_append_printf( url, "&%s=%s", key, s );
		g_free( s );
	}

	_url = g_string_free( url, FALSE );
	TRACE(( "lastfm_url( %s ) = %s\n", method, _url ));
	return _url;
}

static gchar *lastfm_url( const char *method, ... )
{
	va_list args;
	gchar *url;

	va_start( args, method );     /* Initialize variable arguments. */
	url = lastfm_url_v( method, args );
	va_end( args );               /* Reset variable arguments.      */

	return url;
}

static void lastfm_error( GError **error, const char *method, char *content )
{
	if ( error != NULL && *error != NULL )
	{
		struct xml_info info;
		char *xml = xml_find_named( content, "error", &info );
		if ( xml != NULL )
		{
			char *attribs;
			const char *error_description = xml_unbox( &xml, NULL, &attribs );
			const char *error_code = NULL;
			GString *message;

			for (;;)
			{
				char *name;
				xml = xml_unbox_attribute( &attribs, &name );
				if ( xml == NULL ) break;
				if ( strcmp( name, "code" ) == 0 )
				{
					error_code = xml;
					break;
				}
			}

			if ( error_code != NULL ) (*error)->code = atoi( error_code );

			message = g_string_new( ( error_description != NULL ) ? error_description : (*error)->message );
			g_string_append_printf( message, "\n\nmethod = %s", method );
			g_free( (*error)->message );
			(*error)->message = g_string_free( message, FALSE );

			TRACE(( "*** Last.fm error %d \"%s\"\n", (*error)->code, (*error)->message ));
		}
	}
}

static char *lastfm_get( GError **error, const char *method, ... )
{
	gboolean success;
	va_list args;
	gchar *url;
	char *content;

	va_start( args, method );     /* Initialize variable arguments. */
	url = lastfm_url_v( method, args );
	va_end( args );               /* Reset variable arguments.      */

	ASSERT( url != NULL );
	success = http_get( url, 0, NULL, &content, NULL, error );
	g_free( url );

	if ( success ) return content;

	lastfm_error( error, method, content );
	upnp_free_string( content );
	return NULL;
}

static char *lastfm_post( GError **error, const char *method, ... )
{
	gboolean success;
	va_list args;
	gchar *url;
	char *content, *type;

	va_start( args, method );     /* Initialize variable arguments. */
	url = lastfm_url_v( method, args );
	va_end( args );               /* Reset variable arguments.      */

	ASSERT( url != NULL );
	type = "application/x-www-form-urlencoded";
	content = strchr( url, '?' );
	ASSERT( content != NULL );
	*content++ = '\0';
	success = http_post( url, 16 * 1024, &type, &content, NULL, error );
	g_free( url );

	if ( success ) return content;

	lastfm_error( error, method, content );
	upnp_free_string( content );
	return NULL;
}

/*-----------------------------------------------------------------------------------------------*/

gboolean is_lastfm_url( const char *url )
{
	return strncmp( url, lastfm_api_url, sizeof(lastfm_api_url) - 1 ) == 0;
}

gchar *lastfm_album_get_info_url( const char *artist, const char *album )
{
	return lastfm_url( "album.getinfo", "artist", artist, "album", album, NULL );
}

char *lastfm_album_get_info( const char *artist, const char *album, GError **error )
{
	return lastfm_get( error, "album.getinfo", "artist", artist, "album", album, NULL );
}

char *lastfm_auth_get_mobile_session( const char *username, const char *password, GError **error )
{
	gchar *username_lowered = g_utf8_strdown( username, -1 );
	char auth_token[33];
	gchar *api_sig;
	char *result;

	md5cat( strcpy( auth_token, username_lowered ), password );
	g_free( username_lowered );

	api_sig = g_strdup_printf( "api_key%s" "authToken%s" "method%s" "username%s",
		api_key, auth_token, "auth.getmobilesession", username );
	md5cat( api_sig, secret );

	result = lastfm_get( error, "auth.getmobilesession", "username", username, "authToken", auth_token, "api_key", api_key, "api_sig", api_sig, NULL );

	g_free( api_sig );
	return result;
}

char *lastfm_radio_get_playlist( const char *sk, gboolean discovery, GError **error )
{
	const char *discovery_str = (discovery) ? "1" : "0";
	gchar *api_sig;
	char *result;

	api_sig = g_strdup_printf( "api_key%s" "bitrate%s" "buylinks%s" "discovery%s" "method%s" "sk%s",
		api_key, BITRATE, BUYLINKS, discovery_str, "radio.getplaylist", sk );
	md5cat( api_sig, secret );

	result = lastfm_post( error, "radio.getplaylist",
		"discovery", discovery_str, "bitrate", BITRATE, "buylinks", BUYLINKS,
		"api_key", api_key, "sk", sk, "api_sig", api_sig, NULL );

	g_free( api_sig );
	return result;
}

char *lastfm_radio_tune( const char *lang, const char *station, const char *sk, GError **error )
{
	gchar *api_sig;
	char *result;

	if ( lang != NULL )
	{
		api_sig = g_strdup_printf( "api_key%s" "lang%s" "method%s" "sk%s" "station%s",
			api_key, lang, "radio.tune", sk, station );
		md5cat( api_sig, secret );

		result = lastfm_post( error, "radio.tune", "lang", lang, "station", station, "api_key", api_key, "api_sig", api_sig, "sk", sk, NULL );
	}
	else
	{
		api_sig = g_strdup_printf( "api_key%s" "method%s" "sk%s" "station%s",
			api_key, "radio.tune", sk, station );
		md5cat( api_sig, secret );

		result = lastfm_post( error, "radio.tune", "station", station, "api_key", api_key, "sk", sk, "api_sig", api_sig, NULL );
	}

	g_free( api_sig );
	return result;
}

/*-----------------------------------------------------------------------------------------------*/
