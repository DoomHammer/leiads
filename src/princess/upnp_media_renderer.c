/*
	princess/upnp_media_renderer.c
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
#include "upnp_private.h"
#include "upnp_media_renderer.h"

const char upnp_device_MediaRenderer_1[] = "urn:schemas-upnp-org:device:MediaRenderer:1";

/*--- urn:schemas-upnp-org:service:AVTransport:1 ------------------------------------------------*/

const char upnp_serviceId_AVTransport[] = "urn:upnp-org:serviceId:AVTransport";

gboolean AVTransport_SetAVTransportURI( const upnp_device *device, upnp_ui4 InstanceID, const char *CurrentURI, const char *CurrentURIMetaData, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "SetAVTransportURI" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "CurrentURI", (CurrentURI != NULL) ? CurrentURI : "" );
	upnp_add_string_arg( in, "CurrentURIMetaData", (CurrentURIMetaData != NULL) ? CurrentURIMetaData : "" );

	return upnp_action( device, upnp_serviceId_AVTransport, in, NULL, NULL, error );
}

gboolean AVTransport_SetNextAVTransportURI( const upnp_device *device, upnp_ui4 InstanceID, const char *NextURI, const char *NextURIMetaData, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "SetNextAVTransportURI" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "NextURI", (NextURI != NULL) ? NextURI : "" );
	upnp_add_string_arg( in, "NextURIMetaData", (NextURIMetaData != NULL) ? NextURIMetaData : "" );

	return upnp_action( device, upnp_serviceId_AVTransport, in, NULL, NULL, error );
}

gboolean AVTransport_GetMediaInfo( const upnp_device *device, upnp_ui4 InstanceID, upnp_ui4 *NrTracks, upnp_string *MediaDuration, upnp_string *CurrentURI, upnp_string *CurrentURIMetaData, upnp_string *NextURI, upnp_string *NextURIMetaData, upnp_string *PlayMedium, upnp_string *RecordMedium, upnp_string *WriteStatus, GError **error )
{
	upnp_arg in[2], out[10];
	int outc = 10;

	upnp_set_action_name( in, "GetMediaInfo" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	if ( upnp_action( device, upnp_serviceId_AVTransport, in, out, &outc, error ) )
	{
		upnp_assert( outc == 10 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));
		upnp_debug(( "%s = \"%s\"\n", out[4].name, out[4].value ));
		upnp_debug(( "%s = \"%s\"\n", out[5].name, out[5].value ));
		upnp_debug(( "%s = \"%s\"\n", out[6].name, out[6].value ));
		upnp_debug(( "%s = \"%s\"\n", out[7].name, out[7].value ));
		upnp_debug(( "%s = \"%s\"\n", out[8].name, out[8].value ));
		upnp_debug(( "%s = \"%s\"\n", out[9].name, out[9].value ));

		if ( NrTracks != NULL ) *NrTracks = upnp_get_ui4_value( out, 1 );
		if ( MediaDuration != NULL ) *MediaDuration = upnp_get_string_value( out, 2 );
		if ( CurrentURI != NULL ) *CurrentURI = upnp_get_string_value( out, 3 );
		if ( CurrentURIMetaData != NULL ) *CurrentURIMetaData = upnp_get_string_value( out, 4 );
		if ( NextURI != NULL ) *NextURI = upnp_get_string_value( out, 5 );
		if ( NextURIMetaData != NULL ) *NextURIMetaData = upnp_get_string_value( out, 6 );
		if ( PlayMedium != NULL ) *PlayMedium = upnp_get_string_value( out, 7 );
		if ( RecordMedium != NULL ) *RecordMedium = upnp_get_string_value( out, 8 );
		if ( WriteStatus != NULL ) *WriteStatus = upnp_get_string_value( out, 9 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean AVTransport_GetTransportInfo( const upnp_device *device, upnp_ui4 InstanceID, upnp_string *CurrentTransportState, upnp_string *CurrentTransportStatus, upnp_string *CurrentSpeed, GError **error )
{
	upnp_arg in[2], out[4];
	int outc = 4;

	upnp_set_action_name( in, "GetTransportInfo" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	if ( upnp_action( device, upnp_serviceId_AVTransport, in, out, &outc, error ) )
	{
		upnp_assert( outc == 4 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));

		if ( CurrentTransportState != NULL ) *CurrentTransportState = upnp_get_string_value( out, 1 );
		if ( CurrentTransportStatus != NULL ) *CurrentTransportStatus = upnp_get_string_value( out, 2 );
		if ( CurrentSpeed != NULL ) *CurrentSpeed = upnp_get_string_value( out, 3 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean AVTransport_GetPositionInfo( const upnp_device *device, upnp_ui4 InstanceID, upnp_ui4 *Track, upnp_string *TrackDuration, upnp_string *TrackMetaData, upnp_string *TrackURI, upnp_string *RelTime, upnp_string *AbsTime, upnp_i4 *RelCount, upnp_i4 *AbsCount, GError **error )
{
	upnp_arg in[2], out[9];
	int outc = 9;

	upnp_set_action_name( in, "GetPositionInfo" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	if ( upnp_action( device, upnp_serviceId_AVTransport, in, out, &outc, error ) )
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

		if ( Track != NULL ) *Track = upnp_get_ui4_value( out, 1 );
		if ( TrackDuration != NULL ) *TrackDuration = upnp_get_string_value( out, 2 );
		if ( TrackMetaData != NULL ) *TrackMetaData = upnp_get_string_value( out, 3 );
		if ( TrackURI != NULL ) *TrackURI = upnp_get_string_value( out, 4 );
		if ( RelTime != NULL ) *RelTime = upnp_get_string_value( out, 5 );
		if ( AbsTime != NULL ) *AbsTime = upnp_get_string_value( out, 6 );
		if ( RelCount != NULL ) *RelCount = upnp_get_i4_value( out, 7 );
		if ( AbsCount != NULL ) *AbsCount = upnp_get_i4_value( out, 8 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean AVTransport_GetDeviceCapabilities( const upnp_device *device, upnp_ui4 InstanceID, upnp_string *PlayMedia, upnp_string *RecMedia, upnp_string *RecQualityModes, GError **error )
{
	upnp_arg in[2], out[4];
	int outc = 4;

	upnp_set_action_name( in, "GetDeviceCapabilities" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	if ( upnp_action( device, upnp_serviceId_AVTransport, in, out, &outc, error ) )
	{
		upnp_assert( outc == 4 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));

		if ( PlayMedia != NULL ) *PlayMedia = upnp_get_string_value( out, 1 );
		if ( RecMedia != NULL ) *RecMedia = upnp_get_string_value( out, 2 );
		if ( RecQualityModes != NULL ) *RecQualityModes = upnp_get_string_value( out, 3 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean AVTransport_GetTransportSettings( const upnp_device *device, upnp_ui4 InstanceID, upnp_string *PlayMode, upnp_string *RecQualityMode, GError **error )
{
	upnp_arg in[2], out[3];
	int outc = 3;

	upnp_set_action_name( in, "GetTransportSettings" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	if ( upnp_action( device, upnp_serviceId_AVTransport, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( PlayMode != NULL ) *PlayMode = upnp_get_string_value( out, 1 );
		if ( RecQualityMode != NULL ) *RecQualityMode = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean AVTransport_Stop( const upnp_device *device, upnp_ui4 InstanceID, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "Stop" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	return upnp_action( device, upnp_serviceId_AVTransport, in, NULL, NULL, error );
}

gboolean AVTransport_Play( const upnp_device *device, upnp_ui4 InstanceID, const char *Speed, GError **error )
{
	upnp_arg in[3];

	upnp_set_action_name( in, "Play" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "Speed", Speed );

	return upnp_action( device, upnp_serviceId_AVTransport, in, NULL, NULL, error );
}

gboolean AVTransport_Pause( const upnp_device *device, upnp_ui4 InstanceID, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "Pause" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );

	return upnp_action( device, upnp_serviceId_AVTransport, in, NULL, NULL, error );
}


/*--- urn:schemas-upnp-org:service:RenderingControl:1 -------------------------------------------*/

const char upnp_serviceId_RenderingControl[] = "urn:upnp-org:serviceId:RenderingControl";

const char Master[] = "Master";

gboolean RenderingControl_GetMute( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_boolean *CurrentMute, GError **error )
{
	upnp_arg in[3], out[2];
	int outc = 2;

	upnp_set_action_name( in, "GetMute" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "Channel", (Channel != NULL) ? Channel : Master );

	if ( upnp_action( device, upnp_serviceId_RenderingControl, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( CurrentMute != NULL );
		*CurrentMute = upnp_get_boolean_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean RenderingControl_SetMute( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_boolean DesiredMute, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "SetMute" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "Channel", (Channel != NULL) ? Channel : Master );
	upnp_add_boolean_arg( in, "DesiredMute", DesiredMute );

	return upnp_action( device, upnp_serviceId_RenderingControl, in, NULL, NULL, error );
}

gboolean RenderingControl_GetVolume( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_ui2 *CurrentVolume, GError **error )
{
	upnp_arg in[3], out[2];
	int outc = 2;

	upnp_set_action_name( in, "GetVolume" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "Channel", (Channel != NULL) ? Channel : Master );

	if ( upnp_action( device, upnp_serviceId_RenderingControl, in, out, &outc, error ) )
	{
		upnp_assert( outc == 2 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));

		upnp_assert( CurrentVolume != NULL );
		*CurrentVolume = upnp_get_ui2_value( out, 1 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean RenderingControl_SetVolume( const upnp_device *device, upnp_ui4 InstanceID, const char *Channel, upnp_ui2 DesiredVolume, GError **error )
{
	upnp_arg in[4];

	upnp_set_action_name( in, "SetVolume" );
	upnp_add_ui4_arg( in, "InstanceID", InstanceID );
	upnp_add_string_arg( in, "Channel", (Channel != NULL) ? Channel : Master );
	upnp_add_ui2_arg( in, "DesiredVolume", DesiredVolume );

	return upnp_action( device, upnp_serviceId_RenderingControl, in, NULL, NULL, error );
}

/*--- urn:schemas-upnp-org:service:ConnectionManager:1 ------------------------------------------*/

const char upnp_serviceId_ConnectionManager[] = "urn:upnp-org:serviceId:ConnectionManager";

gboolean ConnectionManager_GetProtocolInfo( const upnp_device *device, upnp_string *Source, upnp_string *Sink, GError **error )
{
	upnp_arg in[1], out[3];
	int outc = 3;

	upnp_set_action_name( in, "GetProtocolInfo" );

	if ( upnp_action( device, upnp_serviceId_ConnectionManager, in, out, &outc, error ) )
	{
		upnp_assert( outc == 3 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));

		if ( Source != NULL ) *Source = upnp_get_string_value( out, 1 );
		if ( Sink != NULL ) *Sink = upnp_get_string_value( out, 2 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean ConnectionManager_PrepareForConnection( const upnp_device *device, const char *RemoteProtocolInfo, const char *PeerConnectionManager, upnp_i4 PeerConnectionID, const char *Direction, upnp_i4 *ConnectionID, upnp_i4 *AVTransportID, upnp_i4 *RcsID, GError **error )
{
	upnp_arg in[5], out[4];
	int outc = 4;

	upnp_set_action_name( in, "PrepareForConnection" );
	upnp_add_string_arg( in, "RemoteProtocolInfo", RemoteProtocolInfo );
	upnp_add_string_arg( in, "PeerConnectionManager", PeerConnectionManager );
	upnp_add_i4_arg( in, "PeerConnectionID", PeerConnectionID );
	upnp_add_string_arg( in, "Direction", Direction );

	if ( upnp_action( device, upnp_serviceId_ConnectionManager, in, out, &outc, error ) )
	{
		upnp_assert( outc == 4 );
		upnp_debug(( "%s = \"%s\"\n", out[1].name, out[1].value ));
		upnp_debug(( "%s = \"%s\"\n", out[2].name, out[2].value ));
		upnp_debug(( "%s = \"%s\"\n", out[3].name, out[3].value ));

		upnp_assert( ConnectionID != NULL );
		*ConnectionID = upnp_get_i4_value( out, 1 );
		upnp_assert( AVTransportID != NULL );
		*AVTransportID = upnp_get_i4_value( out, 2 );
		upnp_assert( RcsID != NULL );
		*RcsID = upnp_get_i4_value( out, 3 );

		upnp_release_action( out );
		return TRUE;
	}

	return FALSE;
}

gboolean ConnectionManager_ConnectionComplete( const upnp_device *device, upnp_i4 ConnectionID, GError **error )
{
	upnp_arg in[2];

	upnp_set_action_name( in, "ConnectionComplete" );
	upnp_add_i4_arg( in, "ConnectionID", ConnectionID );

	return upnp_action( device, upnp_serviceId_ConnectionManager, in, NULL, NULL, error );
}

/*===============================================================================================*/
