/*
	princess/upnp_media_server.h
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

extern const char upnp_device_MediaServer_1[] /*= "urn:schemas-upnp-org:device:MediaServer:1"*/;

/*--- urn:schemas-upnp-org:service:ContentDirectory:1 -------------------------------------------*/

extern const char upnp_serviceId_ContentDirectory[] /*= "urn:upnp-org:serviceId:ContentDirectory"*/;

extern const char BrowseMetadata[] /*= "BrowseMetadata"*/;
extern const char BrowseDirectChildren[] /*= "BrowseDirectChildren"*/;

gboolean ContentDirectory_Browse(
	const upnp_device *device, const char *ObjectID, const char *BrowseFlag, const char *Filter,
	upnp_ui4 StartingIndex, upnp_ui4 RequestedCount, const char *SortCriteria,
	upnp_string *Result, upnp_ui4 *NumberReturned, upnp_ui4 *TotalMatches, upnp_ui4 *UpdateID, GError **error );
gboolean ContentDirectory_GetSearchCapabilities( const upnp_device *device, upnp_string *SearchCaps, GError **error );
gboolean ContentDirectory_GetSortCapabilities( const upnp_device *device, upnp_string *SortCaps, GError **error );
gboolean ContentDirectory_GetSystemUpdateID( const upnp_device *device, upnp_ui4 *SystemUpdateID, GError **error );
/*gboolean ContentDirectory_Search( const upnp_device *device, , ... );*/

/*===============================================================================================*/
