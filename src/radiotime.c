/*
	radiotime.c
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

static const char api_url[] = "http://opml.radiotime.com/";

#if RADIOTIME

/*
	NOTE:
	=====

	The file "../../radiotime/partner_id.h" contains a (non-public)
	RadioTime API partner ID & partner authentication key in the following format:

	static const char partner_id[] = "...";
	static const char partner_authentication_key[]  = "...";

	The file "../../radiotime/partner_id.h" is NOT part of this work and is
	therefore one of the prerequisites for building an executable of Leia
	with RadioTime support. If non-existing, simply create this file on your own
	and fill it with YOUR very own RadioTime API partner ID & partner authentication key.

	If you don't have a RadioTime partner ID yet, you can apply for it at
	https://radiotime.com/Services.aspx.

	The RadioTime API Terms of Use can be found at
	http://inside.radiotime.com/developers/terms.
*/

#include "../../radiotime/partner_id.h"

#else

/* The partner_id[] and partner_authentication_key[] of Leia DS */

static const char *partner_id = NULL;
/*static const char *partner_authentication_key = NULL;*/

#endif

/*-----------------------------------------------------------------------------------------------*/

#include "../icons/16x16/radiotime.h"

void RegisterRadioTimeStockIcon( void )
{
	register_stock_icon( LEIADS_STOCK_RADIOTIME, radiotime_16x16 );
}

gchar *GetRadioTimeLogoFilename( void )
{
	return g_strdup( "radiotime.png" );
}

/*-----------------------------------------------------------------------------------------------*/

const char *GetRadioTimeTitle( void )
{
	if ( Settings->radiotime.active == 1 && Settings->radiotime.username[0] )
	{
		return Text(RADIOTIME_PRESETS);
	}
	else if ( Settings->radiotime.active == 2 )
	{
		const upnp_device *device = GetAVTransport();

		if ( upnp_device_is( device, linn_serviceId_Radio ) || upnp_device_is( device, openhome_serviceId_Radio ) )
			return Text(RADIOTIME_DS_PRESETS);
	}

	return NULL;
}

gboolean IsRadioTimeItem( const char *didl_item )
{
	char id[12];

	return ( xml_get_named_attribute( didl_item, "id", id, sizeof(id) ) != NULL &&
	         strncmp( id, "radiotime", 9 ) == 0 );
}

/*-----------------------------------------------------------------------------------------------*/

struct radio_time_outline
{
	const char *type;
	const char *text;
	const char *URL;
	const char *bitrate;
	const char *reliability;
	const char *guide_id;
	const char *subtext;
	const char *genre_id;
	const char *formats;
	const char *show_id;
	const char *item;
	const char *image;
	const char *current_track, *now_playing_id;
	const char *preset_number, *preset_id, *is_preset;
};

static GList *get_radio_time_folder( GList *content_list, char *t )
{
	char *name, *attributes, *s;

	while ( (s = xml_unbox( &t, &name, &attributes )) != NULL )
	{
		if ( strcmp( name, "outline" ) == 0 )
		{
			struct radio_time_outline outline = {0};

			for (;;)
			{
				const char *value = xml_unbox_attribute( &attributes, &name );
				if ( value == NULL ) break;

				TRACE(( "get_radio_time_folder(): %s = \"%s\"\n", name, value ));
				if ( strcmp( name, "type" ) == 0 ) outline.type = value;
				else if ( strcmp( name, "text" ) == 0 ) outline.text = value;
				else if ( strcmp( name, "URL" ) == 0 ) outline.URL = value;
				else if ( strcmp( name, "bitrate" ) == 0 ) outline.bitrate = value;
				else if ( strcmp( name, "reliability" ) == 0 ) outline.reliability = value;
				else if ( strcmp( name, "guide_id" ) == 0 ) outline.guide_id = value;
				else if ( strcmp( name, "subtext" ) == 0 ) outline.subtext = value;
				else if ( strcmp( name, "genre_id" ) == 0 ) outline.genre_id = value;
				else if ( strcmp( name, "formats" ) == 0 ) outline.formats = value;
				else if ( strcmp( name, "show_id" ) == 0 ) outline.show_id = value;
				else if ( strcmp( name, "item" ) == 0 ) outline.item = value;
				else if ( strcmp( name, "image" ) == 0 ) outline.image = value;
				else if ( strcmp( name, "current_track" ) == 0 ) outline.current_track = value;
				else if ( strcmp( name, "now_playing_id" ) == 0 ) outline.now_playing_id = value;
				else if ( strcmp( name, "preset_number" ) == 0 ) outline.preset_number = value;
				else if ( strcmp( name, "preset_id" ) == 0 ) outline.preset_id = value;
				else if ( strcmp( name, "is_preset" ) == 0 ) outline.is_preset = value;
				else TRACE(( "* get_radio_time_folder(): unknown name \"%s\"\n", name ));
			}

			if ( outline.type == NULL )
			{
				content_list = get_radio_time_folder( content_list, s );
			}
			else
			{
				struct radio_time_outline *o = g_new( struct radio_time_outline, 1 );
				*o = outline;
				content_list = g_list_append( content_list, o );
			}
		}
	}

	return content_list;
}

gboolean is_radio_time_folder( const struct radio_time_outline *o )
{
	return ( o->item == NULL && strcmp( o->type, "link" ) == 0 );
}

gint radio_time_compare_func( const struct radio_time_outline *a, const struct radio_time_outline *b )
{
	int i, j;

	if ( is_radio_time_folder( a ) )
		i = -1;
	else if ( a->preset_number != NULL )
		i = atoi( a->preset_number );
	else
		i = 0x7FFF;

	if ( is_radio_time_folder( b ) )
		j = -1;
	else if ( b->preset_number != NULL )
		j = atoi( b->preset_number );
	else
		j = 0x7FFF;

	if ( i != j ) return i - j;

	return strcmp( (a->text != NULL) ? a->text : "", (b->text != NULL) ? b->text : "" );
}

struct xml_content *GetRadioTimeFolder( const char *id, const char *formats, gchar **title, GError **error )
{
	GString *content_str = NULL;
	int n;

	TRACE(( "-> GetRadioTimeFolder( \"%s\" )\n", id ));

	if ( Settings->radiotime.active == 1 )
	{
		GString *url = g_string_new( api_url );
		GList *content_list = NULL, *l;
		char *content, *name, *s;
		gboolean success;

		s = strchr( id, ':' );
		if ( s == NULL )
			g_string_append( url, "Browse.ashx?c=presets" );
		else
			g_string_append_printf( url, "Browse.ashx?id=%s", s + 1 );

		if ( formats != NULL ) g_string_append_printf( url, "&formats=%s", formats );

		g_string_append_printf( url, "&partnerId=%s&username=%s&locale=%s",
			partner_id, Settings->radiotime.username, GetLanguage() );

		TRACE(( "GetRadioTimeFolder(): url = %s\n", url->str ));
		success = http_get( url->str, 0, NULL, &content, NULL, error );
		g_string_free( url, TRUE );

		if ( !success )
		{
			TRACE(( "<- GetRadioTimeFolder() = NULL [http_get() failed]\n" ));
			return NULL;
		}

#if LOGLEVEL > 2
		{
			gchar *log_filename = build_logfilename( "leiads-radiotime", "xml" );
			if ( log_filename != NULL )
			{
				gboolean success = g_file_set_contents( log_filename, content, -1, NULL );
				if ( success ) TRACE(( "%s written successfully\n", log_filename ));
				else TRACE(( "*** %s not written successfully\n", log_filename ));
				g_free( log_filename );
			}
		}
#endif

		s = xml_unbox( &content, &name, NULL );
		if ( strcmp( name, "opml" ) == 0 )
		{
			for (;;)
			{
				char *t = xml_unbox( &s, &name, NULL );
				if ( t == NULL ) break;

				if ( strcmp( name, "head" ) == 0 && title != NULL )
				{
					char *u;

					while ( (u = xml_unbox( &t, &name, NULL )) != NULL )
					{
						if ( strcmp( name, "title" ) == 0 )
						{
							if ( *title != NULL )
							{
								TRACE(( "GetRadioTimeFolder(): Replace title \"%s\" with \"%s\"", *title, u ));
								g_free( *title );
							}
							else
							{
								TRACE(( "GetRadioTimeFolder(): Set title to \"%s\"", u ));
							}

							*title = g_strdup( u );
							break;
						}
					}
				}
				else if ( strcmp( name, "body" ) == 0 )
				{
					content_list = get_radio_time_folder( content_list, t );
				}
			}
		}

		content_list = g_list_sort( content_list, (GCompareFunc)radio_time_compare_func );

		content_str = g_string_new( "" );
		n = 0;

		for ( l = g_list_first( content_list ); l != NULL; l = g_list_next( l ) )
		{
			struct radio_time_outline *o = (struct radio_time_outline *)l->data;

			if ( is_radio_time_folder( o ) )
			{
				g_string_append_printf( content_str, "<item id=\"radiotime:%s\">", (o->guide_id != NULL) ? o->guide_id : "0" );

				xml_string_append_boxed( content_str, "dc:title", o->text, NULL );
				/*xml_string_append_boxed( content_str, "res", o->URL, NULL );*/

				g_string_append( content_str, "<upnp:class>object.container</upnp:class></item>" );
				n++;
			}
			else if ( o->item != NULL && (strcmp( o->type, "audio" ) == 0 || strcmp( o->type, "link" ) == 0) )
			{
				g_string_append_printf( content_str, "<item id=\"radiotime:%s:%s\">",
					(o->preset_id != NULL) ? o->preset_id : "0", (o->now_playing_id != NULL) ? o->now_playing_id : "0" );

				xml_string_append_boxed( content_str, "dc:title", o->text, NULL );
				xml_string_append_boxed( content_str, "res", o->URL, "bitrate", o->bitrate, NULL );
				xml_string_append_boxed( content_str, "upnp:albumArtURI", o->image, NULL );
				xml_string_append_boxed( content_str, "upnp:originalTrackNumber", o->preset_number, NULL );
				xml_string_append_boxed( content_str, "radiotime:subtext", o->subtext, NULL );
				xml_string_append_boxed( content_str, "radiotime:bitrate", o->bitrate, NULL );
				xml_string_append_boxed( content_str, "radiotime:reliability", o->reliability, NULL );
				xml_string_append_boxed( content_str, "radiotime:current_track", o->current_track, NULL );

				g_string_append( content_str, "<upnp:class>object.item.audioItem" );
				if ( o->item != NULL /*&& strcmp( o->item, "station" ) == 0*/ )
					g_string_append( content_str, ".audioBroadcast" );
				g_string_append( content_str, "</upnp:class></item>" );
				n++;
			}
			else
			{
				TRACE(( "*** get_radio_time_folder(): unknown outline\n" ));
			}

			g_free( o );
		}

		g_list_free( content_list );
		upnp_free_string( content );
	}
	else if ( Settings->radiotime.active == 2 )
	{
		const upnp_device *device = GetAVTransport();
		char *id_array, *id_list, *metadata_list, *s;
		gboolean success;
		int i;

		if ( upnp_device_is( device, linn_serviceId_Radio ) )
		{
			success = Linn_Radio_IdArray( device, NULL, &id_array, error );
		}
		else if ( upnp_device_is( device, openhome_serviceId_Radio ) )
		{
			success = Openhome_Radio_IdArray( device, NULL, &id_array, error );
		}
		else
		{
			g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, Text(ERRMSG_INTERNAL) );
			success = FALSE;
		}

		if ( !success )
		{
			TRACE(( "<- GetRadioTimeFolder() = NULL [Radio_IdArray() failed]\n" ));
			return NULL;
		}

		TRACE(( "get_radio_folder(): id_array[] = %s\n", id_array ));
		n = decode64( (unsigned char *)id_array, id_array );
		ASSERT( sizeof( upnp_ui4 ) == 4 );
		ASSERT( n % 4 == 0 );
		n /= 4;
		TRACE(( "get_radio_folder(): n = %d\n", n ));

		id_list = g_new( char, n * (10 /*strlen("4294967295")*/ + 1 /*strlen(",")*/) );
		s = id_list; *s = '\0';

		if ( upnp_device_is( device, linn_serviceId_Radio ) )
		{
			for ( i = 0; i < n; i++ )
			{
				upnp_ui4 id = GUINT32_FROM_BE( ((upnp_ui4 *)id_array)[i] );
				if ( id != 0 )
				{
					if ( s != id_list ) *s++ = ',';
					s += sprintf( s, "%lu", (unsigned long)id );
				}
			}
			TRACE(( "get_radio_folder(): id_list = \"%s\"\n", id_list ));

			upnp_free_string( id_array );

			success = Linn_Radio_ReadList( device, id_list, &metadata_list, error );
		}
		else if ( upnp_device_is( device, openhome_serviceId_Radio ) )
		{
			for ( i = 0; i < n; i++ )
			{
				upnp_ui4 id = GUINT32_FROM_BE( ((upnp_ui4 *)id_array)[i] );
				if ( id != 0 )
				{
					if ( s != id_list ) *s++ = ' ';
					s += sprintf( s, "%lu", (unsigned long)id );
				}
			}
			TRACE(( "get_radio_folder(): id_list = \"%s\"\n", id_list ));

			upnp_free_string( id_array );

			success = Openhome_Radio_ReadList( device, id_list, &metadata_list, error );
		}
		else
		{
			g_set_error( error, UPNP_ERROR, UPNP_ERR_SERVICE_NOT_FOUND, Text(ERRMSG_INTERNAL) );
			success = FALSE;
		}

		g_free( id_list );
		if ( !success )
		{
			TRACE(( "<- GetRadioTimeFolder() = NULL [Radio_ReadList() failed]\n" ));
			return NULL;
		}

		/*TRACE(( "get_radio_folder(): metadata_list = %p = \"%s\"\n", (void*)metadata_list, metadata_list ));*/
#if LOGLEVEL > 2
		{
			gchar *log_filename = build_logfilename( "leiads-radiotime", "xml" );
			if ( log_filename != NULL )
			{
				gboolean success = g_file_set_contents( log_filename, metadata_list, -1, NULL );
				if ( success ) TRACE(( "%s written successfully\n", log_filename ));
				else TRACE(( "*** %s not written successfully\n", log_filename ));
				g_free( log_filename );
			}
		}
#endif

		content_str = g_string_new( "" );
		n = 0;

		for (;;)
		{
			upnp_ui4 id = 0;
			char *track = PlaylistData_GetTrack( &metadata_list, NULL, &id );
			if ( track == NULL || id == 0 ) break;
			/*TRACE(( "§§§ %s\n", track ));*/

			track = MetaDataList_GetItem( &track );
			if ( track != NULL )
			{
				TRACE(( "§§§ %s\n", track ));

				s = strstr( track, "</item>" );
				if ( s != NULL && strstr( track, "<upnp:originalTrackNumber>" ) == NULL )
					sprintf( s, "<upnp:originalTrackNumber>%lu</upnp:originalTrackNumber></item>", (unsigned long)id );

				g_string_append( content_str, track );
				n++;
			}
		}

		TRACE(( "get_radio_folder(): metadata_list = %p = \"%s\"\n", (void *)metadata_list, metadata_list ));
		upnp_free_string( metadata_list );
	}

	TRACE(( "<- GetRadioTimeFolder() = ok\n" ));
	return xml_content_from_string( content_str, n );
}

/*-----------------------------------------------------------------------------------------------*/
