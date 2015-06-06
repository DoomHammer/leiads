/*
	princess/linn_source.c
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

#include "upnp.h"
#include "upnp_private.h"  /* for debug stuff */
#include "linn_source.h"

/*=== urn:linn-co-uk:device:Source:1 ============================================================*/

/*const char linn_device_Source_1[] = "urn:linn-co-uk:device:Source:1";*/

/*=== urn:linn-co-uk:serviceId:Volkano ==========================================================*/

const char linn_serviceId_Volkano[] = "urn:linn-co-uk:serviceId:Volkano";

/*--- urn:linn-co-uk:service:Volkano:1 ----------------------------------------------------------*/

gboolean Linn_Volkano_UglyName( const upnp_device *device, upnp_string *aUglyName, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "UglyName" );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aUglyName != NULL );
		*aUglyName = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_MacAddress( const upnp_device *device, upnp_string *aMacAddress, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "MacAddress" );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aMacAddress != NULL );
		*aMacAddress = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_ProductId( const upnp_device *device, upnp_string *aProductNumber, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "ProductId" );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aProductNumber != NULL );
		*aProductNumber = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_BoardId( const upnp_device *device, upnp_ui4 aIndex, upnp_string *aBoardNumber, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "BoardId" );
	upnp_add_ui4_arg( in, "aIndex", aIndex );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aBoardNumber != NULL );
		*aBoardNumber = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_BoardType( const upnp_device *device, upnp_ui4 aIndex, upnp_string *aBoardNumber, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "BoardType" );
	upnp_add_ui4_arg( in, "aIndex", aIndex );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aBoardNumber != NULL );
		*aBoardNumber = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_MaxBoards( const upnp_device *device, upnp_ui4 *aMaxBoards, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "MaxBoards" );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aMaxBoards != NULL );
		*aMaxBoards = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Volkano_SoftwareVersion( const upnp_device *device, upnp_string *aSoftwareVersion, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "SoftwareVersion" );

	if ( upnp_action( device, linn_serviceId_Volkano, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aSoftwareVersion != NULL );
		*aSoftwareVersion = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:linn-co-uk:serviceId:Product ==========================================================*/

const char linn_serviceId_Product[] = "urn:linn-co-uk:serviceId:Product";

/*--- urn:linn-co-uk:service:Product:1 ----------------------------------------------------------*/

gboolean Linn_Product_Room( const upnp_device *device, upnp_string *aRoom, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "Room" );

	if ( upnp_action( device, linn_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aRoom != NULL );
		*aRoom = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Product_SetRoom( const upnp_device *device, const char *aRoom, GError **error )
{
	upnp_arg in[2];

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "SetRoom" );
	upnp_add_string_arg( in, "aRoom", aRoom );

	return upnp_action( device, linn_serviceId_Product, in, NULL, NULL, error );
}

gboolean Linn_Product_Standby( const upnp_device *device, upnp_boolean *aStandby, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "Standby" );

	if ( upnp_action( device, linn_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aStandby != NULL );
		*aStandby = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Product_SetStandby( const upnp_device *device, upnp_boolean aStandby, GError **error )
{
	upnp_arg in[2];

	if ( device != NULL && device->parent != NULL ) device = device->parent;  /* Auskerry */

	upnp_set_action_name( in, "SetStandby" );
	upnp_add_boolean_arg( in, "aStandby", aStandby );

	return upnp_action( device, linn_serviceId_Product, in, NULL, NULL, error );
}

/*=== urn:linn-co-uk:serviceId:Media (Auskerry & Bute) ==========================================*/

const char linn_serviceId_Media[] = "urn:linn-co-uk:serviceId:Media";

const char Relativity_Absolute[] = "Absolute";
const char Relativity_Relative[] = "Relative";

/*--- urn:linn-co-uk:service:Media:1 ------------------------------------------------------------*/

gboolean Linn_Media_Play( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Play" );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_Pause( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Pause" );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_Stop( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Stop" );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_SeekSeconds( const upnp_device *device, upnp_i4 aOffset, const char *aRelativity, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SeekSeconds" );
	upnp_add_i4_arg( in, "aOffset", aOffset );
	upnp_add_string_arg( in, "aRelativity", aRelativity );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_SeekTracks( const upnp_device *device, upnp_i4 aOffset, const char *aRelativity, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SeekTracks" );
	upnp_add_i4_arg( in, "aOffset", aOffset );
	upnp_add_string_arg( in, "aRelativity", aRelativity );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_Status( const upnp_device *device, upnp_string *aTransportState,
	upnp_i4 *aCurrentTrackIndex, upnp_ui4 *aCurrentTrackSeconds, upnp_ui4 *aCurrentTrackDuration,
	upnp_ui4 *aCurrentTrackBitDepth, upnp_ui4 *aCurrentTrackSampleRate, GError **error )
{
	upnp_arg in[1], out[7];
	int outc = 7;

	upnp_set_action_name( in, "Status" );

	if ( upnp_action( device, linn_serviceId_Media, in, out, &outc, error ) )
	{
		upnp_assert( outc == 7 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));
		upnp_debug(( "%s = \"%s\"\n", out[5].name, out[5].value ));
		upnp_debug(( "%s = \"%s\"\n", out[6].name, out[6].value ));

		if ( aTransportState != NULL ) *aTransportState = upnp_get_string_value( out, 1 );
		if ( aCurrentTrackIndex != NULL ) *aCurrentTrackIndex = upnp_get_i4_value( out, 2 );
		if ( aCurrentTrackSeconds != NULL ) *aCurrentTrackSeconds = upnp_get_ui4_value( out, 3 );
		if ( aCurrentTrackDuration != NULL ) *aCurrentTrackDuration = upnp_get_ui4_value( out, 4 );
		if ( aCurrentTrackBitDepth != NULL ) *aCurrentTrackBitDepth = upnp_get_ui4_value( out, 5 );
		if ( aCurrentTrackSampleRate != NULL ) *aCurrentTrackSampleRate = upnp_get_ui4_value( out, 6 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Media_PlaylistInsert( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, char *aPlaylistData, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "PlaylistInsert" );
	upnp_add_ui4_arg( in, "aSeq", aSeq );
	upnp_add_ui4_arg( in, "aIndex", aIndex );
	upnp_add_string_arg( in, "aPlaylistData", aPlaylistData );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_PlaylistDelete( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, upnp_ui4 aCount, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "PlaylistDelete" );
	upnp_add_ui4_arg( in, "aSeq", aSeq );
	upnp_add_ui4_arg( in, "aIndex", aIndex );
	upnp_add_ui4_arg( in, "aCount", aCount );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

#if 0  /* was never implemented by Linn */
gboolean Linn_Media_PlaylistMove( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndexFrom, upnp_ui4 aIndexTo, upnp_ui4 aCount, GError **error )
{
	upnp_arg in[5];

	upnp_set_action_name( in, "PlaylistMove" );
	upnp_add_ui4_arg( in, "aSeq", aSeq );
	upnp_add_ui4_arg( in, "aIndexFrom", aIndexFrom );
	upnp_add_ui4_arg( in, "aIndexTo", aIndexTo );
	upnp_add_ui4_arg( in, "aCount", aCount );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, errror );
}
#endif

gboolean Linn_Media_PlaylistRead( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 aIndex, upnp_ui4 aCount, upnp_string *aPlaylistData, GError **error )
{
	upnp_arg in[4], out[2];
	int outc = 2;

	struct part_list
	{
		struct part_list* next;
		char data[2];
	} *list = NULL, *element;

	/* unfortunately the upnp action "PlaylistRead" has a nasty limit of 50 tracks,
	   so we need to do some nasty stuff to get around this */

	while ( aCount > 50 )
	{
		int count = aCount % 50;
		if ( count == 0 ) count = 50;

		if ( !Linn_Media_PlaylistRead( device, aSeq, aIndex + (aCount - count), count, aPlaylistData, error ) )
		{
			while ( list != NULL )
			{
				element = list;
				list = element->next;
				upnp_free_string( (upnp_string)element );
			}

			return FALSE;
		}

		upnp_assert( offsetof( struct part_list, next ) == 0 );
		list_prepend( &list, (struct part_list*)*aPlaylistData );
		aCount -= count;
	}

	upnp_set_action_name( in, "PlaylistRead" );
	upnp_add_ui4_arg( in, "aSeq", aSeq );
	upnp_add_ui4_arg( in, "aIndex", aIndex );
	upnp_add_ui4_arg( in, "aCount", aCount );

	if ( upnp_action( device, linn_serviceId_Media, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		/*upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));*/

		upnp_assert( aPlaylistData != NULL );
		*aPlaylistData = upnp_get_string_value( out, 1 );

		while ( list != NULL )
		{
			char* s;

			element = list;
			list = element->next;

			s = strstr( *aPlaylistData, endPlaylistData );
			upnp_assert( s != NULL );
			if ( s != NULL )
			{
				struct buffer *buf = get_action_buffer( out );
				size_t offset;

				upnp_assert( buf != NULL );

				upnp_assert( *aPlaylistData > buf->addr );
				offset = *aPlaylistData - buf->addr;
				upnp_assert( offset < buf->size );

				buf->len = s - buf->addr;
				upnp_assert( buf->len < buf->size );
				append_string_to_buffer( buf, (char*)element + strlen_beginPlaylistData );

				*aPlaylistData = buf->addr + offset;
			}

			upnp_free_string( (upnp_string)element );
		}

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Media_PlaylistSequence( const upnp_device *device, upnp_ui4 *aSeq, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "PlaylistSequence" );

	if ( upnp_action( device, linn_serviceId_Media, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aSeq != NULL );
		*aSeq = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Media_PlaylistTrackCount( const upnp_device *device, upnp_ui4 aSeq, upnp_ui4 *aTrackCount, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "PlaylistTrackCount" );
	upnp_add_ui4_arg( in, "aSeq", aSeq );

	if ( upnp_action( device, linn_serviceId_Media, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aTrackCount != NULL );
		*aTrackCount = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Media_SetNonSeekableBuffer( const upnp_device *device, upnp_ui4 aBytes, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetNonSeekableBuffer" );
	upnp_add_ui4_arg( in, "aBytes", aBytes );

	return upnp_action( device, linn_serviceId_Media, in, NULL, NULL, error );
}

gboolean Linn_Media_NonSeekableBuffer( const upnp_device *device, upnp_ui4 *aBytes, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "NonSeekableBuffer" );

	if ( upnp_action( device, linn_serviceId_Media, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aBytes != NULL );
		*aBytes = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*-----------------------------------------------------------------------------------------------*/

const char beginPlaylistData[] = "<PlaylistData>";
const char endPlaylistData[]  = "</PlaylistData>";

struct PlaylistData *PlaylistData_New( int Count, GError **error )
{
	struct PlaylistData *PlaylistData;

	PlaylistData = (struct PlaylistData*)alloc_buffer( Count * 2048, 64 * 2024, "PlaylistData", error );
	if ( PlaylistData != NULL ) PlaylistData_Reset( PlaylistData );
	return PlaylistData;
}

gboolean PlaylistData_Reset( struct PlaylistData *PlaylistData )
{
	struct buffer *buffer = (struct buffer*)PlaylistData;

	upnp_assert( PlaylistData != NULL );
	if ( PlaylistData == NULL ) return FALSE;

	reset_buffer( buffer );
	return append_string_to_buffer( buffer, beginPlaylistData );
}

gboolean PlaylistData_AddTrack( struct PlaylistData *PlaylistData, const char *MetaData_Contents, GError **error )
{
	struct buffer *buffer = (struct buffer*)PlaylistData;
	char tmp[UPNP_MAX_URL], *uri;
	int result = TRUE;

	upnp_assert( PlaylistData != NULL && MetaData_Contents != NULL );
	if ( PlaylistData == NULL || MetaData_Contents == NULL ) return FALSE;

	uri = xml_get_named( MetaData_Contents, "res", tmp, sizeof(tmp) );
	/*if ( uri == NULL ) uri = _xml_get_named( MetaData_Contents, "ns:res", tmp, sizeof(tmp) );*/
	if ( uri != NULL )
	{
		append_string_to_buffer( buffer, "<Track><Uri>" );
		xml_wrap_delimiters_to_buffer( buffer, uri );
		append_string_to_buffer( buffer, "</Uri><MetaData>" );

		if ( xml_get_name( MetaData_Contents, tmp, sizeof(tmp) ) != NULL )
		{
			if ( strcmp( tmp, "DIDL-Lite" ) == 0 )
			{
				xml_wrap_delimiters_to_buffer( buffer, MetaData_Contents );
			}
			else if ( strcmp( tmp, "item" ) == 0 )
			{
				append_string_to_buffer( buffer, "&lt;DIDL-Lite "
					"xmlns:dc=&quot;http://purl.org/dc/elements/1.1/&quot; "
					"xmlns:upnp=&quot;urn:schemas-upnp-org:metadata-1-0/upnp/&quot; "
					"xmlns:dlna=&quot;urn:schemas-dlna-org:metadata-1-0/&quot; " );

				/* TODO:
				   Filter the meta data and only append those we got a corresponding
				   xmlns schema for */
				if ( strstr( MetaData_Contents, "<arib:" ) != NULL || strstr( MetaData_Contents, " arib:" ) != NULL )
					append_string_to_buffer( buffer,
						"xmlns:arib=&quot;urn:schemas-arib-or-jp:elements-1-0/&quot; " );
				if ( strstr( MetaData_Contents, "<dtcp:" ) != NULL || strstr( MetaData_Contents, " dtcp:" ) != NULL )
					append_string_to_buffer( buffer,
						"xmlns:dtcp=&quot;urn:schemas-dtcp-com:metadata-1-0/&quot; " );
				if ( strstr( MetaData_Contents, "<pv:" ) != NULL || strstr( MetaData_Contents, " pv:" ) != NULL )
					append_string_to_buffer( buffer,
						"xmlns:pv=&quot;http://www.pv.com/pvns/&quot; " );
				if ( strstr( MetaData_Contents, "<lastfm:" ) != NULL || strstr( MetaData_Contents, " lastfm:" ) != NULL )
					append_string_to_buffer( buffer,
						"xmlns:lastfm=&quot;http://leia.sommerfeldt.f-m.fm/lastfmns/&quot; " );
				append_string_to_buffer( buffer,
					"xmlns=&quot;urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/&quot;&gt;" );

				xml_wrap_delimiters_to_buffer( buffer, MetaData_Contents );
				append_string_to_buffer( buffer, "&lt;/DIDL-Lite&gt;" );
			}
			else
			{
				upnp_warning(( "PlaylistData_AddTrack(): Not a valid music track (3)\n" ));
				g_set_error( error, UPNP_ERROR, -3, "Not a valid music track" );
				result = FALSE;
			}

			append_string_to_buffer( buffer, "</MetaData></Track>" );
		}
		else
		{
			upnp_warning(( "PlaylistData_AddTrack(): Not a valid music track (2)\n" ));
			g_set_error( error, UPNP_ERROR, -2, "Not a valid music track" );
			result = FALSE;
		}
	}
	else
	{
		upnp_warning(( "Media_PlaylistData_AddTrack(): Not a valid music track (1)\n" ));
		g_set_error( error, UPNP_ERROR, -1, "Not a valid music track" );
		result = FALSE;
	}

	return result;
}

char *PlaylistData_Finalize( struct PlaylistData *PlaylistData, GError **error )
{
	struct buffer *buffer = (struct buffer*)PlaylistData;

	upnp_assert( PlaylistData != NULL );

	if ( !append_string_to_buffer( buffer, endPlaylistData ) )
	{
		g_set_error( error, UPNP_ERROR, UPNP_ERR_OUT_OF_MEMORY, "Out of memory" );
		return NULL;
	}

	return buffer->addr;
}

void PlaylistData_Free( struct PlaylistData *PlaylistData )
{
	struct buffer *buffer = (struct buffer*)PlaylistData;

	upnp_assert( PlaylistData != NULL );

	release_buffer( buffer );
}

char *PlaylistData_GetTrack( char **PlaylistData, char **Uri, upnp_ui4 *Id )
{
	char *name, *content;

	upnp_assert( PlaylistData != NULL );
	if ( PlaylistData == NULL || *PlaylistData == NULL || **PlaylistData == '\0' ) return NULL;

	content = xml_unbox( PlaylistData, &name, NULL );
	if ( content == NULL ) return NULL;

	if ( strcmp( name, "PlaylistData" ) == 0 || strcmp( name, "MetaDataList" ) == 0 ||
	     strcmp( name, "TrackList" ) == 0 /* Davaar */ || strcmp( name, "ChannelList" ) == 0 /* Davaar */ )
	{
		*PlaylistData = content;
		content = xml_unbox( PlaylistData, &name, NULL );
		if ( content == NULL ) return NULL;
	}

	if ( strcmp( name, "Track" ) == 0 || strcmp( name, "Entry" ) == 0 /* Davaar */ )
	{
		char *uri = NULL, *meta_data = NULL;
		upnp_ui4 id = 0UL;

		for (;;)
		{
			char *s = xml_unbox( &content, &name, NULL );
			if ( s == NULL ) break;
			if ( strcmp( name, "Id" ) == 0 )
			{
				id = (upnp_ui4)strtoul( s, NULL, 10 );
			}
			else if ( strcmp( name, "Uri" ) == 0 )
			{
				uri = s;
			}
			else if ( strcmp( name, "MetaData" ) == 0 || strcmp( name, "Metadata" ) == 0 )
			{
				meta_data = s;
			}
			else
			{
				upnp_warning(( "PlaylistData_GetTrack(): unknown tag = \"%s\"\n", name ));
			}
		}

		if ( Uri != NULL ) *Uri = uri;
		if ( Id  != NULL ) *Id  = id;
		return meta_data;
	}

	upnp_warning(( "PlaylistData_GetTrack(): unknown name = \"%s\"\n", name ));
	return NULL;
}

/*=== urn:linn-co-uk:serviceId:Playlist (Cara) ==================================================*/

const char linn_serviceId_Playlist[] = "urn:linn-co-uk:serviceId:Playlist";
const char TransportState_Buffering[] = "Buffering";

/*--- urn:linn-co-uk:service:Playlist:1 ---------------------------------------------------------*/

gboolean Linn_Playlist_Read( const upnp_device *device, upnp_ui4 aId, upnp_string *aUri, upnp_string *aMetaData, GError **error )
{
	upnp_arg in[2], out[3];
	int outc = 3;

	upnp_set_action_name( in, "Read" );
	upnp_add_ui4_arg( in, "aId", aId );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( aUri != NULL ) *aUri = upnp_get_string_value( out, 1 );
		if ( aMetaData != NULL ) *aMetaData = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_ReadList( const upnp_device *device, const char *aIdList, upnp_string *aMetaDataList, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ReadList" );
	upnp_add_string_arg( in, "aIdList", aIdList );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		/*upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));*/

		upnp_assert( aMetaDataList != NULL );
		*aMetaDataList = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_Insert( const upnp_device *device, upnp_ui4 aAfterId, const char *aUri, const char *aMetaData, upnp_ui4 *aNewId, GError **error )
{
	upnp_arg in[4], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Insert" );
	upnp_add_ui4_arg( in, "aAfterId", aAfterId );
	upnp_add_string_arg( in, "aUri", aUri );
	upnp_add_string_arg( in, "aMetaData", aMetaData );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aNewId != NULL );
		*aNewId = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_Delete( const upnp_device *device, upnp_ui4 aId, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "Delete" );
	upnp_add_ui4_arg( in, "aId", aId );

	return upnp_action( device, linn_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Linn_Playlist_DeleteAll( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "DeleteAll" );

	return upnp_action( device, linn_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Linn_Playlist_SetRepeat( const upnp_device *device, upnp_boolean aRepeat, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetRepeat" );
	upnp_add_boolean_arg( in, "aRepeat", aRepeat );

	return upnp_action( device, linn_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Linn_Playlist_Repeat( const upnp_device *device, upnp_boolean *aRepeat, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Repeat" );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aRepeat != NULL );
		*aRepeat = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_SetShuffle( const upnp_device *device, upnp_boolean aShuffle, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetShuffle" );
	upnp_add_boolean_arg( in, "aShuffle", aShuffle );

	return upnp_action( device, linn_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Linn_Playlist_Shuffle( const upnp_device *device, upnp_boolean *aShuffle, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Shuffle" );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aShuffle != NULL );
		*aShuffle = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_TracksMax( const upnp_device *device, upnp_ui4 *aTracksMax, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "TracksMax" );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aTracksMax != NULL );
		*aTracksMax = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_IdArray( const upnp_device *device, upnp_ui4 *aIdArrayToken, upnp_string* aIdArray, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "IdArray" );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( aIdArrayToken != NULL ) *aIdArrayToken = upnp_get_ui4_value( out, 1 );
		if ( aIdArray != NULL ) *aIdArray = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Playlist_IdArrayChanged( const upnp_device *device, upnp_ui4 aIdArrayToken, int *aIdArrayChanged, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "IdArrayChanged" );
	upnp_add_ui4_arg( in, "aIdArrayToken", aIdArrayToken );

	if ( upnp_action( device, linn_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aIdArrayChanged != NULL );
		*aIdArrayChanged = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:linn-co-uk:serviceId:Ds (Cara) ========================================================*/

const char linn_serviceId_Ds[] = "urn:linn-co-uk:serviceId:Ds";

/*--- urn:linn-co-uk:service:Ds:1 ---------------------------------------------------------------*/

gboolean Linn_Ds_Play( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Play" );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_Pause( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Pause" );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_Stop( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Stop" );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 aSecond, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondAbsolute" );
	upnp_add_ui4_arg( in, "aSecond", aSecond );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_SeekSecondRelative( const upnp_device *device, upnp_i4 aSecond, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondRelative" );
	upnp_add_i4_arg( in, "aSecond", aSecond );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_SeekTrackId( const upnp_device *device, upnp_ui4 aTrackId, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekTrackId" );
	upnp_add_ui4_arg( in, "aTrackId", aTrackId );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_SeekTrackAbsolute( const upnp_device *device, upnp_ui4 aTrack, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekTrackAbsolute" );
	upnp_add_ui4_arg( in, "aTrack", aTrack );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_SeekTrackRelative( const upnp_device *device, upnp_i4 aTrack, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekTrackRelative" );
	upnp_add_i4_arg( in, "aTrack", aTrack );

	return upnp_action( device, linn_serviceId_Ds, in, NULL, NULL, error );
}

gboolean Linn_Ds_State( const upnp_device *device, upnp_string *aTransportState,
		upnp_ui4 *aTrackDuration, upnp_ui4 *aTrackBitRate, gboolean *aTrackLossless,
		upnp_ui4 *aTrackBitDepth, upnp_ui4 *aTrackSampleRate, upnp_string *aTrackCodecName,
		upnp_ui4 *aTrackId, GError **error )
{
	upnp_arg in[1], out[9];
	int outc = 9;

	upnp_set_action_name( in, "State" );

	if ( upnp_action( device, linn_serviceId_Ds, in, out, &outc, error ) )
	{
		upnp_assert( outc == 9 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));
		upnp_debug(( "%s = \"%s\"\n", out[5].name, out[5].value ));
		upnp_debug(( "%s = \"%s\"\n", out[6].name, out[6].value ));
		upnp_debug(( "%s = \"%s\"\n", out[7].name, out[7].value ));
		upnp_debug(( "%s = \"%s\"\n", out[8].name, out[8].value ));

		if ( aTransportState != NULL ) *aTransportState = upnp_get_string_value( out, 1 );
		if ( aTrackDuration != NULL ) *aTrackDuration = upnp_get_ui4_value( out, 2 );
		if ( aTrackBitRate != NULL ) *aTrackBitRate = upnp_get_ui4_value( out, 3 );
		if ( aTrackLossless != NULL ) *aTrackLossless = upnp_get_boolean_value( out, 4 );
		if ( aTrackBitDepth != NULL ) *aTrackBitDepth = upnp_get_ui4_value( out, 5 );
		if ( aTrackSampleRate != NULL ) *aTrackSampleRate = upnp_get_ui4_value( out, 6 );
		if ( aTrackCodecName != NULL ) *aTrackCodecName = upnp_get_string_value( out, 7 );
		if ( aTrackId != NULL ) *aTrackId = upnp_get_ui4_value( out, 8 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Ds_ProtocolInfo( const upnp_device *device, upnp_string *aSupportedProtocols, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ProtocolInfo" );

	if ( upnp_action( device, linn_serviceId_Ds, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aSupportedProtocols != NULL );
		*aSupportedProtocols = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:linn-co-uk:serviceId:Radio (Cara 6) ===================================================*/

const char linn_serviceId_Radio[] = "urn:linn-co-uk:serviceId:Radio";

/*--- urn:linn-co-uk:service:Radio:1 ------------------------------------------------------------*/

gboolean Linn_Radio_Play( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Play" );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_Pause( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Pause" );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_Stop( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Stop" );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 aSecond, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondAbsolute" );
	upnp_add_ui4_arg( in, "aSecond", aSecond );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_SeekSecondRelative( const upnp_device *device, upnp_i4 aSecond, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondRelative" );
	upnp_add_i4_arg( in, "aSecond", aSecond );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_Channel( const upnp_device *device, upnp_string *aUri, upnp_string *aMetadata, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "Channel" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( aUri != NULL ) *aUri = upnp_get_string_value( out, 1 );
		if ( aMetadata != NULL ) *aMetadata = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_SetChannel( const upnp_device *device, const char *aUri, const char *aMetadata, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SetChannel" );
	upnp_add_string_arg( in, "aUri", aUri );
	upnp_add_string_arg( in, "aMetadata", aMetadata );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_ProtocolInfo( const upnp_device *device, upnp_string *aInfo, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ProtocolInfo" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aInfo != NULL );
		*aInfo = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_TransportState( const upnp_device *device, upnp_string *aState, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "TransportState" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aState != NULL );
		*aState = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_Id( const upnp_device *device, upnp_ui4 *aId, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Id" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aId != NULL );
		*aId = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_SetId( const upnp_device *device, upnp_ui4 aId, const char *aUri, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SetId" );
	upnp_add_ui4_arg( in, "aId", aId );
	upnp_add_string_arg( in, "aUri", aUri );

	return upnp_action( device, linn_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Linn_Radio_Read( const upnp_device *device, upnp_ui4 aId, upnp_string *aMetadata, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Read" );
	upnp_add_ui4_arg( in, "aId", aId );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aMetadata != NULL );
		*aMetadata = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_ReadList( const upnp_device *device, const char *aIdList, upnp_string *aMetadataList, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ReadList" );
	upnp_add_string_arg( in, "aIdList", aIdList );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aMetadataList != NULL );
		*aMetadataList = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_IdArray( const upnp_device *device, upnp_ui4 *aIdArrayToken, upnp_string *aIdArray, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "IdArray" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( aIdArrayToken != NULL ) *aIdArrayToken = upnp_get_ui4_value( out, 1 );
		if ( aIdArray != NULL ) *aIdArray = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_IdArrayChanged( const upnp_device *device, upnp_ui4 aIdArrayToken, upnp_boolean *aIdArrayChanged, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "IdArrayChanged" );
	upnp_add_ui4_arg( in, "aIdArrayToken", aIdArrayToken );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aIdArrayChanged != NULL );
		*aIdArrayChanged = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Linn_Radio_IdsMax( const upnp_device *device, upnp_ui4 *aIdsMax, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "IdsMax" );

	if ( upnp_action( device, linn_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( aIdsMax != NULL );
		*aIdsMax = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:av-openhome-org:serviceId:Product (Davaar) ============================================*/

const char openhome_serviceId_Product[] = "urn:av-openhome-org:serviceId:Product";

/*--- urn:av-openhome-org:service:Product:1 -----------------------------------------------------*/

gboolean Openhome_Manufacturer( const upnp_device *device, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error )
{
	upnp_arg in[1], out[5];
	int outc = 9;

	upnp_set_action_name( in, "Manufacturer" );

	if ( upnp_action( device, openhome_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 5 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));

		if ( Name != NULL ) *Name = upnp_get_string_value( out, 1 );
		if ( Info != NULL ) *Info = upnp_get_string_value( out, 2 );
		if ( Url != NULL ) *Url = upnp_get_string_value( out, 3 );
		if ( ImageUri != NULL ) *ImageUri = upnp_get_string_value( out, 4 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Model( const upnp_device *device, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error )
{
	upnp_arg in[1], out[5];
	int outc = 9;

	upnp_set_action_name( in, "Model" );

	if ( upnp_action( device, openhome_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 5 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));

		if ( Name != NULL ) *Name = upnp_get_string_value( out, 1 );
		if ( Info != NULL ) *Info = upnp_get_string_value( out, 2 );
		if ( Url != NULL ) *Url = upnp_get_string_value( out, 3 );
		if ( ImageUri != NULL ) *ImageUri = upnp_get_string_value( out, 4 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Product( const upnp_device *device, upnp_string *Room, upnp_string *Name, upnp_string *Info, upnp_string *Url, upnp_string *ImageUri, GError **error )
{
	upnp_arg in[1], out[6];
	int outc = 9;

	upnp_set_action_name( in, "Product" );

	if ( upnp_action( device, openhome_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 6 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));
		upnp_debug(( "%s = \"%s\"\n", out[5].name, out[5].value ));

		if ( Room != NULL ) *Room = upnp_get_string_value( out, 1 );
		if ( Name != NULL ) *Name = upnp_get_string_value( out, 2 );
		if ( Info != NULL ) *Info = upnp_get_string_value( out, 3 );
		if ( Url != NULL ) *Url = upnp_get_string_value( out, 4 );
		if ( ImageUri != NULL ) *ImageUri = upnp_get_string_value( out, 5 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Standby( const upnp_device *device, upnp_boolean *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Standby" );

	if ( upnp_action( device, openhome_serviceId_Product, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_SetStandby( const upnp_device *device, upnp_boolean Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetStandby" );
	upnp_add_boolean_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Product, in, NULL, NULL, error );
}

/*=== urn:av-openhome-org:serviceId:Playlist (Davaar) ===========================================*/

const char openhome_serviceId_Playlist[] = "urn:av-openhome-org:serviceId:Playlist";

/*--- urn:av-openhome-org:service:Playlist:1 ----------------------------------------------------*/

gboolean Openhome_Playlist_Play( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Play" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Pause( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Pause" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Stop( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Stop" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Next( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Next" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Previous( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Previous" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_SetRepeat( const upnp_device *device, upnp_boolean Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetRepeat" );
	upnp_add_boolean_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Repeat( const upnp_device *device, upnp_boolean *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Repeat" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_SetShuffle( const upnp_device *device, upnp_boolean Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SetShuffle" );
	upnp_add_boolean_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_Shuffle( const upnp_device *device, upnp_boolean *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Shuffle" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondAbsolute" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_SeekSecondRelative( const upnp_device *device, upnp_i4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondRelative" );
	upnp_add_i4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_SeekId( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekId" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_SeekIndex( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekIndex" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_TransportState( const upnp_device *device, upnp_string *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "TransportState" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_Id( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Id" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_Read( const upnp_device *device, upnp_ui4 Id, upnp_string *Uri, upnp_string *Metadata, GError **error )
{
	upnp_arg in[2], out[3];
	int outc = 3;

	upnp_set_action_name( in, "Read" );
	upnp_add_ui4_arg( in, "Id", Id );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( Uri != NULL ) *Uri = upnp_get_string_value( out, 1 );
		if ( Metadata != NULL ) *Metadata = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_ReadList( const upnp_device *device, upnp_string IdList, upnp_string *TrackList, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ReadList" );
	upnp_add_string_arg( in, "IdList", IdList );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		/*upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));*/

		upnp_assert( TrackList != NULL );
		*TrackList = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_Insert( const upnp_device *device, upnp_ui4 AfterId, upnp_string Uri, upnp_string Metadata, upnp_ui4 *NewId, GError **error )
{
	upnp_arg in[4], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Insert" );
	upnp_add_ui4_arg( in, "AfterId", AfterId );
	upnp_add_string_arg( in, "Uri", Uri );
	upnp_add_string_arg( in, "Metadata", Metadata );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( NewId != NULL );
		*NewId = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_DeleteId( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "DeleteId" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_DeleteAll( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "DeleteAll" );

	return upnp_action( device, openhome_serviceId_Playlist, in, NULL, NULL, error );
}

gboolean Openhome_Playlist_TracksMax( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "TracksMax" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_IdArray( const upnp_device *device, upnp_ui4 *Token, upnp_string *Array, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "IdArray" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( Token != NULL ) *Token = upnp_get_ui4_value( out, 1 );
		if ( Array != NULL ) *Array = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_IdArrayChanged( const upnp_device *device, upnp_ui4 Token, upnp_boolean *Value, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "IdArrayChanged" );
	upnp_add_ui4_arg( in, "Token", Token );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Playlist_ProtocolInfo( const upnp_device *device, upnp_string *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ProtocolInfo" );

	if ( upnp_action( device, openhome_serviceId_Playlist, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*=== urn:av-openhome-org:serviceId:Radio (Davaar) ==============================================*/

const char openhome_serviceId_Radio[] = "urn:av-openhome-org:serviceId:Radio";

/*--- urn:av-openhome-org:service:Radio:1 -------------------------------------------------------*/

gboolean Openhome_Radio_Play( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Play" );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_Pause( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Pause" );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_Stop( const upnp_device *device, GError **error )
{
	upnp_arg in[1];

	upnp_set_action_name( in, "Stop" );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_SeekSecondAbsolute( const upnp_device *device, upnp_ui4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondAbsolute" );
	upnp_add_ui4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_SeekSecondRelative( const upnp_device *device, upnp_i4 Value, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "SeekSecondRelative" );
	upnp_add_i4_arg( in, "Value", Value );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_Channel( const upnp_device *device, upnp_string *Uri, upnp_string *Metadata, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "Channel" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( Uri != NULL ) *Uri = upnp_get_string_value( out, 1 );
		if ( Metadata != NULL ) *Metadata = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_SetChannel( const upnp_device *device, const char *Uri, const char *Metadata, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SetChannel" );
	upnp_add_string_arg( in, "Uri", Uri );
	upnp_add_string_arg( in, "Metadata", Metadata );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_TransportState( const upnp_device *device, upnp_string *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "TransportState" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_Id( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Id" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_SetId( const upnp_device *device, upnp_ui4 Value, const char *Uri, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "SetId" );
	upnp_add_ui4_arg( in, "Value", Value );
	upnp_add_string_arg( in, "Uri", Uri );

	return upnp_action( device, openhome_serviceId_Radio, in, NULL, NULL, error );
}

gboolean Openhome_Radio_Read( const upnp_device *device, upnp_ui4 Id, upnp_string *Metadata, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "Read" );
	upnp_add_ui4_arg( in, "Id", Id );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Metadata != NULL );
		*Metadata = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_ReadList( const upnp_device *device, const char *IdList, upnp_string *ChannelList, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ReadList" );
	upnp_add_string_arg( in, "IdList", IdList );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( ChannelList != NULL );
		*ChannelList = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_IdArray( const upnp_device *device, upnp_ui4 *Token, upnp_string *Array, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "IdArray" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( Token != NULL ) *Token = upnp_get_ui4_value( out, 1 );
		if ( Array != NULL ) *Array = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_IdArrayChanged( const upnp_device *device, upnp_ui4 Token, upnp_boolean *Value, GError **error )
{
	upnp_arg in[2], out[2];
	int outc = 2;

	upnp_set_action_name( in, "IdArrayChanged" );
	upnp_add_ui4_arg( in, "Token", Token );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_ChannelsMax( const upnp_device *device, upnp_ui4 *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ChannelsMax" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean Openhome_Radio_ProtocolInfo( const upnp_device *device, upnp_string *Value, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "ProtocolInfo" );

	if ( upnp_action( device, openhome_serviceId_Radio, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( Value != NULL );
		*Value = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*===============================================================================================*/
