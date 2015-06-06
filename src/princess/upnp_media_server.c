/*
	princess/upnp_media_server.c
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

#include "upnp.h"
#include "upnp_private.h"  /* for debug stuff */
#include "upnp_media_server.h"

const char upnp_device_MediaServer_1[] = "urn:schemas-upnp-org:device:MediaServer:1";

/*--- urn:schemas-upnp-org:service:ContentDirectory:1 -------------------------------------------*/

const char upnp_serviceId_ContentDirectory[] = "urn:upnp-org:serviceId:ContentDirectory";

const char BrowseMetadata[] = "BrowseMetadata";
const char BrowseDirectChildren[] = "BrowseDirectChildren";

gboolean ContentDirectory_Browse( const upnp_device *device, const char *ObjectID, const char *BrowseFlag, const char *Filter,
	upnp_ui4 StartingIndex, upnp_ui4 RequestedCount, const char *SortCriteria,
	upnp_string *Result, upnp_ui4 *NumberReturned, upnp_ui4 *TotalMatches, upnp_ui4 *UpdateID, GError **error )
{
	upnp_arg in[7], out[5];
	int outc = 5;

	upnp_set_action_name( in, "Browse" );
	upnp_add_string_arg( in, "ObjectID", ObjectID );
	upnp_add_string_arg( in, "BrowseFlag", BrowseFlag );
	upnp_add_string_arg( in, "Filter", Filter );
	upnp_add_ui4_arg( in, "StartingIndex", StartingIndex );
	upnp_add_ui4_arg( in, "RequestedCount", RequestedCount );
	upnp_add_string_arg( in, "SortCriteria", SortCriteria );

	if ( upnp_action( device, upnp_serviceId_ContentDirectory, in, out, &outc, error ) )
	{
		upnp_assert( outc == 5 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));

		if ( Result != NULL ) *Result = upnp_get_string_value( out, 1 );
		if ( NumberReturned != NULL ) *NumberReturned = upnp_get_ui4_value( out, 2 );
		if ( TotalMatches != NULL ) *TotalMatches = upnp_get_ui4_value( out, 3 );
		if ( UpdateID != NULL ) *UpdateID = upnp_get_ui4_value( out, 4 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean ContentDirectory_GetSearchCapabilities( const upnp_device *device, upnp_string *SearchCaps, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "GetSearchCapabilities" );

	if ( upnp_action( device, upnp_serviceId_ContentDirectory, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( SearchCaps != NULL );
		*SearchCaps = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean ContentDirectory_GetSortCapabilities( const upnp_device *device, upnp_string *SortCaps, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "GetSortCapabilities" );
	if ( upnp_action( device, upnp_serviceId_ContentDirectory, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( SortCaps != NULL );
		*SortCaps = upnp_get_string_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean ContentDirectory_GetSystemUpdateID( const upnp_device *device, upnp_ui4 *SystemUpdateID, GError **error )
{
	upnp_arg in[1], out[2];
	int outc = 2;

	upnp_set_action_name( in, "GetSystemUpdateID" );

	if ( upnp_action( device, upnp_serviceId_ContentDirectory, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( SystemUpdateID != NULL );
		*SystemUpdateID = upnp_get_ui4_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

/*gboolean ContentDirectory_Search( const upnp_device *device, ... );*/

/*===============================================================================================*/
